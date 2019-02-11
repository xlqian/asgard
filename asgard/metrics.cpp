#include "metrics.h"
#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <valhalla/midgard/logging.h>

namespace asgard {

InFlightGuard::InFlightGuard(prometheus::Gauge* gauge) : gauge(gauge) {
    if (gauge != nullptr) {
        gauge->Increment();
    }
}

InFlightGuard::~InFlightGuard() {
    if (gauge != nullptr) {
        gauge->Decrement();
    }
}

InFlightGuard::InFlightGuard(InFlightGuard&& other) {
    this->gauge = other.gauge;
    other.gauge = nullptr;
}

void InFlightGuard::operator=(InFlightGuard&& other) {
    this->gauge = other.gauge;
    other.gauge = nullptr;
}

static prometheus::Histogram::BucketBoundaries create_exponential_buckets(double start, double factor, int count) {
    //boundaries need to be sorted!
    auto bucket_boundaries = prometheus::Histogram::BucketBoundaries{};
    double v = start;
    for (auto i = 0; i < count; i++) {
        bucket_boundaries.push_back(v);
        v = v * factor;
    }
    return bucket_boundaries;
}

static prometheus::Histogram::BucketBoundaries create_fixed_duration_buckets() {
    //boundaries need to be sorted!
    auto bucket_boundaries = prometheus::Histogram::BucketBoundaries{
        0.01, 0.02, 0.05, 0.1, 0.2, 0.3, 0.4, 0.5, 0.7, 1, 1.5, 2, 5, 10};
    return bucket_boundaries;
}

Metrics::Metrics(const boost::optional<std::string>& endpoint, const std::string& coverage) {
    if (endpoint == boost::none) {
        return;
    }
    LOG_INFO("metrics available at http://" + *endpoint + "/metrics");
    exposer = std::make_unique<prometheus::Exposer>(*endpoint);
    registry = std::make_shared<prometheus::Registry>();
    exposer->RegisterCollectable(registry);

    auto& histogram_family = prometheus::BuildHistogram()
                                 .Name("asgard_request_duration_seconds")
                                 .Help("duration of request in seconds")
                                 .Labels({{"coverage", coverage}})
                                 .Register(*registry);
    auto desc = pbnavitia::API_descriptor();
    for (int i = 0; i < desc->value_count(); ++i) {
        auto value = desc->value(i);
        // Asgard only manages direct_path and street_network_routing_matrix
        if (value->name() == "direct_path" || value->name() == "street_network_routing_matrix") {
            auto& histo = histogram_family.Add({{"api", value->name()}}, create_fixed_duration_buckets());
            this->request_histogram[static_cast<pbnavitia::API>(value->number())] = &histo;
        }
    }
    auto& in_flight_family = prometheus::BuildGauge()
                                 .Name("asgard_request_in_flight")
                                 .Help("Number of requests currently beeing processed")
                                 .Labels({{"coverage", coverage}})
                                 .Register(*registry);
    this->in_flight = &in_flight_family.Add({});

    this->handle_direct_path_histogram = &prometheus::BuildHistogram()
                                              .Name("asgard_direct_path_duration_seconds")
                                              .Help("duration of direct path computation")
                                              .Labels({{"coverage", coverage}})
                                              .Register(*registry)
                                              .Add({}, create_exponential_buckets(1, 2, 10));

    this->handle_matrix_histogram = &prometheus::BuildHistogram()
                                         .Name("asgard_handle_matrix_duration_seconds")
                                         .Help("duration of matrix computation")
                                         .Labels({{"coverage", coverage}})
                                         .Register(*registry)
                                         .Add({}, create_exponential_buckets(1, 2, 10));
}

InFlightGuard Metrics::start_in_flight() const {
    if (!registry) {
        return InFlightGuard(nullptr);
    }
    return InFlightGuard(this->in_flight);
}

void Metrics::observe_api(pbnavitia::API api, double duration) const {
    if (!registry) {
        return;
    }
    auto it = this->request_histogram.find(api);
    if (it != std::end(this->request_histogram)) {
        it->second->Observe(duration);
    } else {
        LOG_WARN("api " + pbnavitia::API_Name(api) + " not found in metrics");
    }
}

void Metrics::observe_handle_direct_path(double duration) const {
    if (!registry) {
        return;
    }
    this->handle_direct_path_histogram->Observe(duration);
}

void Metrics::observe_handle_matrix(double duration) const {
    if (!registry) {
        return;
    }
    this->handle_matrix_histogram->Observe(duration);
}

} // namespace asgard
