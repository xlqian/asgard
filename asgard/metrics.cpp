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

static prometheus::Histogram::BucketBoundaries create_fixed_duration_buckets() {
    //boundaries need to be sorted!
    auto bucket_boundaries = prometheus::Histogram::BucketBoundaries{
        0.01, 0.02, 0.05, 0.1, 0.2, 0.3, 0.4, 0.5, 0.7, 1, 1.5, 2, 5, 10};
    return bucket_boundaries;
}

Metrics::Metrics(const boost::optional<std::string>& endpoint) {
    if (endpoint == boost::none) {
        return;
    }
    LOG_INFO("metrics available at http://" + *endpoint + "/metrics");
    exposer = std::make_unique<prometheus::Exposer>(*endpoint);
    registry = std::make_shared<prometheus::Registry>();
    exposer->RegisterCollectable(registry);

    auto& in_flight_family = prometheus::BuildGauge()
                                 .Name("asgard_request_in_flight")
                                 .Help("Number of requests currently beeing processed")
                                 .Register(*registry);
    this->in_flight = &in_flight_family.Add({});

    auto& direct_path_family = prometheus::BuildHistogram()
                                   .Name("asgard_direct_path_duration_seconds")
                                   .Help("duration of direct path computation")
                                   .Register(*registry);

    auto& matrix_family = prometheus::BuildHistogram()
                              .Name("asgard_handle_matrix_duration_seconds")
                              .Help("duration of matrix computation")
                              .Register(*registry);

    std::vector<std::string> list_modes = {"walking", "bike", "car"};
    for (auto const& mode : list_modes) {
        LOG_WARN("mode = " + mode);
        auto& histo_direct_path = direct_path_family.Add({{"mode", mode}}, create_fixed_duration_buckets());
        this->handle_direct_path_histogram[mode] = &histo_direct_path;
        auto& histo_matrix = matrix_family.Add({{"mode", mode}}, create_fixed_duration_buckets());
        this->handle_matrix_histogram[mode] = &histo_matrix;
    }
}

InFlightGuard Metrics::start_in_flight() const {
    if (!registry) {
        return InFlightGuard(nullptr);
    }
    return InFlightGuard(this->in_flight);
}

void Metrics::observe_handle_direct_path(const std::string& mode, double duration) const {
    if (!registry) {
        return;
    }
    auto it = this->handle_direct_path_histogram.find(mode);
    if (it != std::end(this->handle_direct_path_histogram)) {
        it->second->Observe(duration);
    } else {
        LOG_WARN("mode " + mode + " not found in metrics");
    }
}

void Metrics::observe_handle_matrix(const std::string& mode, double duration) const {
    if (!registry) {
        return;
    }
    auto it = this->handle_matrix_histogram.find(mode);
    if (it != std::end(this->handle_matrix_histogram)) {
        it->second->Observe(duration);
    } else {
        LOG_WARN("mode " + mode + " not found in metrics");
    }
}

} // namespace asgard
