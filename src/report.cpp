/*******************************************************************************
 * Copyright 2017 MINRES Technologies GmbH
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
/*
 * report.cpp
 *
 *  Created on: 19.09.2017
 *      Author: ubuntu
 */

#include "scc/report.h"

#include <sr_report/sr_report.h>

using namespace sc_core;

static const std::string compose_message(const sc_report &rep) {
    std::stringstream os;
    auto *srr = dynamic_cast<sr_report *>(const_cast<sc_report *>(&rep));
    if (srr) {
        for (std::vector<v::pair>::const_iterator iter = srr->pairs.begin(); iter != srr->pairs.end(); iter++) {
            os << '[' << iter->name << ':';
            switch (iter->type) {
            case v::pair::INT32:
                os << std::hex << std::setfill('0') << "0x" << std::setw(8) << boost::any_cast<int32_t>(iter->data);
                break;
            case v::pair::UINT32:
                os << std::hex << std::setfill('0') << "0x" << std::setw(8) << boost::any_cast<uint32_t>(iter->data);
                break;
            case v::pair::INT64:
                os << std::hex << std::setfill('0') << "0x" << std::setw(16) << boost::any_cast<int64_t>(iter->data);
                break;
            case v::pair::UINT64:
                os << std::hex << std::setfill('0') << "0x" << std::setw(16) << boost::any_cast<uint64_t>(iter->data);
                break;
            case v::pair::STRING:
                os << boost::any_cast<std::string>(iter->data).c_str();
                break;
            case v::pair::BOOL:
                os << (boost::any_cast<bool>(iter->data) ? "true" : "false");
                break;
            case v::pair::DOUBLE:
                os << boost::any_cast<double>(iter->data);
                break;
            case v::pair::TIME:
                os << boost::any_cast<sc_core::sc_time>(iter->data).to_string();
                break;
            default:
                os << boost::any_cast<int32_t>(iter->data);
            }
            os << ']';
        }
        os << ' ';
    }
    if (rep.get_id() >= 0)
        os << "("
           << "IWEF"[rep.get_severity()] << rep.get_id() << ") ";
    os << rep.get_msg_type();
    if (*rep.get_msg()) os << ": " << rep.get_msg();
    if (rep.get_severity() > SC_INFO) {
        char line_number_str[16];
        os << " [FILE:" << rep.get_file_name() << ":" << rep.get_line_number() << "]";
        sc_simcontext *simc = sc_get_curr_simcontext();
        if (simc && sc_is_running()) {
            const char *proc_name = rep.get_process_name();
            if (proc_name) os << "[PROCESS:" << proc_name << "]";
        }
    }
    return os.str();
}

static void report_handler(const sc_report &rep, const sc_actions &actions) {
    const logging::log_level map[] = {logging::INFO, logging::WARNING, logging::ERROR, logging::FATAL};
    if (actions & SC_DISPLAY)
        if (map[rep.get_severity()] <= logging::Log<logging::Output2FILE<logging::SystemC>>::reporting_level() &&
            logging::Output2FILE<logging::SystemC>::stream())
            scc::Log<logging::Output2FILE<logging::SystemC>>().get(map[rep.get_severity()], "SystemC")
                << compose_message(rep);
    //    if ( (actions & SC_LOG) && log_file_name ) {
    //        if ( !log_stream ) log_stream = new ::std::ofstream(log_file_name);
    //        // ios::trunc
    //        *log_stream << rep.get_time() << ": " <<
    //        my_report_compose_message(rep) << ::std::endl;
    //    }
    if (actions & SC_STOP) sc_stop();
    if (actions & SC_ABORT) abort();
    if (actions & SC_THROW) throw rep;
}

void scc::init_logging() { sc_report_handler::set_handler(report_handler); }
