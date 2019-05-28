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

struct AsgardConf;

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
    prometheus::Gauge* in_flight;
    prometheus::Gauge* status_family;
    std::map<const std::string, prometheus::Histogram*> handle_direct_path_histogram;
    std::map<const std::string, prometheus::Histogram*> handle_matrix_histogram;

public:
    Metrics(const AsgardConf& conf);
    InFlightGuard start_in_flight() const;

    void observe_handle_direct_path(const std::string&, double duration) const;
    void observe_handle_matrix(const std::string&, double duration) const;
};

} // namespace asgard
