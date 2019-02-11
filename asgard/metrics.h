#pragma once

#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/gauge.h>
#include "asgard/request.pb.h"
#include <boost/optional.hpp>
#include <boost/utility.hpp>
#include <map>
#include <memory>

//forward declare
namespace prometheus {
class Registry;
class Counter;
class Histogram;
} // namespace prometheus

namespace asgard {

class InFlightGuard {
    prometheus::Gauge* gauge;

public:
    explicit InFlightGuard(prometheus::Gauge* gauge);
    InFlightGuard(InFlightGuard& other) = delete;
    InFlightGuard(InFlightGuard&& other);
    void operator=(InFlightGuard& other) = delete;
    void operator=(InFlightGuard&& other);
    ~InFlightGuard();
};

class Metrics : boost::noncopyable {
protected:
    std::unique_ptr<prometheus::Exposer> exposer;
    std::shared_ptr<prometheus::Registry> registry;
    std::map<pbnavitia::API, prometheus::Histogram*> request_histogram;
    prometheus::Gauge* in_flight;
    prometheus::Histogram* handle_direct_path_histogram;
    prometheus::Histogram* handle_matrix_histogram;

public:
    Metrics(const boost::optional<std::string>& endpoint, const std::string& coverage);
    void observe_api(pbnavitia::API api, double duration) const;
    InFlightGuard start_in_flight() const;

    void observe_handle_direct_path(double duration) const;
    void observe_handle_matrix(double duration) const;
};

} // namespace asgard
