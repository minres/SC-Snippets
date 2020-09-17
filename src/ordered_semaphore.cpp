/*******************************************************************************
 * Copyright 2019 MINRES Technologies GmbH
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *******************************************************************************/

#include "sysc/communication/sc_communication_ids.h"
#include "sysc/kernel/sc_wait.h"
#include "scc/report.h"
#include <scc/ordered_semaphore.h>
#include <util/strprintf.h>

namespace scc {

using namespace sc_core;

// ----------------------------------------------------------------------------
//  CLASS : sc_semaphore
//
//  The sc_semaphore primitive channel class.
// ----------------------------------------------------------------------------

// error reporting
namespace {
std::string gen_unique_event_name(const char* modifier) {
    auto str = std::string("$$$$kernel_event$$$$_") + "_" + modifier;
    return std::string(sc_core::sc_gen_unique_name(str.c_str(), false));
}
} // namespace

void ordered_semaphore::set_capacity(unsigned c) {
    if(typeid(*this)==typeid(ordered_semaphore)){
        auto diff = static_cast<int>(capacity) - static_cast<int>(c);
        capacity=c;
        value-=diff;
        if(value>0) free_evt.notify();
    } else {
        SCCWARN(SCMOD) << "cannot resize fixed size ordered semaphore";
    }
}

void ordered_semaphore::report_error(const char* id, const char* add_msg) const {
    auto msg = add_msg? util::strprintf("semaphore '%s'", name()):util::strprintf("%s: semaphore '%s'", add_msg, name());
    SC_REPORT_ERROR(id, msg.c_str());
}

// constructors

ordered_semaphore::ordered_semaphore(unsigned init_value_)
: sc_core::sc_object(sc_core::sc_gen_unique_name("semaphore"))
, free_evt(gen_unique_event_name("free_event").c_str())
, value(init_value_)
, capacity(init_value_) {
    if(value < 0) {
        report_error(sc_core::SC_ID_INVALID_SEMAPHORE_VALUE_);
    }
}

ordered_semaphore::ordered_semaphore(const char* name_, unsigned init_value_)
: sc_object(name_)
, free_evt(gen_unique_event_name("free_event").c_str())
, value(init_value_)
, capacity(init_value_){
}

// interface methods

// lock (take) the semaphore, block if not available

int ordered_semaphore::wait() {
    queue.push_back(sc_core::sc_get_current_process_handle());
    while(in_use()) {
        sc_core::wait(free_evt);
    }
    --value;
    return 0;
}

// lock (take) the semaphore, return -1 if not available

int ordered_semaphore::trywait() {
    if(in_use()) {
        return -1;
    }
    --value;
    return 0;
}

// unlock (give) the semaphore

int ordered_semaphore::post() {
    if(value==capacity)
        SCCWARN(SCMOD)<<"post() called on entirely free semaphore!";
    else
        ++value;
    if(value>0) free_evt.notify();
    return 0;
}

} // namespace scc
