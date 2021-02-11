#pragma once

#include <axi/axi_tlm.h>
#include <scv4tlm/tlm_rec_initiator_socket.h>
#include <scc/initiator_mixin.h>
#include <tlm/tlm_mm.h>
#include <scc/report.h>
#include <tlm/tlm_id.h>

#include <systemc>
#include <tlm>

#include <unordered_map>
#include <memory>
#include <queue>


// TODO: check aquire/release
namespace axi_bfm {

template <unsigned int BUSWIDTH = 32, unsigned int ADDRWIDTH = 32, unsigned int IDWIDTH = 32>
class axi_pin2tlm_adaptor : public sc_core::sc_module {
public:
    using payload_type = axi::axi_protocol_types::tlm_payload_type;
    using phase_type = axi::axi_protocol_types::tlm_phase_type;

    SC_HAS_PROCESS(axi_pin2tlm_adaptor);

    axi_pin2tlm_adaptor(sc_core::sc_module_name nm);

    scc::initiator_mixin<scv4tlm::tlm_rec_initiator_socket<BUSWIDTH,axi::axi_protocol_types>,axi::axi_protocol_types> output_socket{"output_socket"};

    sc_core::sc_in<bool> clk_i{"clk_i"};
    sc_core::sc_in<bool> resetn_i{"resetn_i"}; // active low reset

    // Write address channel signals
    sc_core::sc_in<sc_dt::sc_uint<IDWIDTH>> aw_id_i{"aw_id_i"};
    sc_core::sc_in<sc_dt::sc_uint<ADDRWIDTH>> aw_addr_i{"aw_addr_i"};
    sc_core::sc_out<bool> aw_ready_o{"aw_ready_o"};
    sc_core::sc_in<bool> aw_lock_i{"aw_lock_i"};
    sc_core::sc_in<bool> aw_valid_i{"aw_valid_i"};
    sc_core::sc_in<sc_dt::sc_uint<3>> aw_prot_i{"aw_prot_i"};
    sc_core::sc_in<sc_dt::sc_uint<3>> aw_size_i{"aw_size_i"};
    sc_core::sc_in<sc_dt::sc_uint<4>> aw_cache_i{"aw_cache_i"};
    sc_core::sc_in<sc_dt::sc_uint<2>> aw_burst_i{"aw_burst_i"};
    sc_core::sc_in<sc_dt::sc_uint<4>> aw_qos_i{"Aaw_qos_i"};
    sc_core::sc_in<sc_dt::sc_uint<4>> aw_region_i{"aw_region_i"};
    sc_core::sc_in<sc_dt::sc_uint<8>> aw_len_i{"aw_len_i"};

    // write data channel signals
    sc_core::sc_in<sc_dt::sc_biguint<BUSWIDTH>> w_data_i{"w_data_i"};
    sc_core::sc_in<sc_dt::sc_uint<BUSWIDTH/8>> w_strb_i{"w_strb_i"};
    sc_core::sc_in<bool> w_last_i{"w_last_i"};
    sc_core::sc_in<bool> w_valid_i{"w_valid_i"};
    sc_core::sc_out<bool> w_ready_o{"w_ready_o"};

    // write response channel signals
    sc_core::sc_out<bool> b_valid_o{"b_valid_o"};
    sc_core::sc_in<bool> b_ready_i{"b_ready_i"};
    sc_core::sc_out<sc_dt::sc_uint<IDWIDTH>> b_id_o{"b_id_o"};
    sc_core::sc_out<sc_dt::sc_uint<2>> b_resp_o{"b_resp_o"};

    // read address channel signals
    sc_core::sc_in<sc_dt::sc_uint<IDWIDTH>> ar_id_i{"ar_id_i"};
    sc_core::sc_in<sc_dt::sc_uint<ADDRWIDTH>> ar_addr_i{"ar_addr_i"};
    sc_core::sc_in<sc_dt::sc_uint<8>> ar_len_i{"ar_len_i"};
    sc_core::sc_in<sc_dt::sc_uint<3>> ar_size_i{"ar_size_i"};
    sc_core::sc_in<sc_dt::sc_uint<2>> ar_burst_i{"ar_burst_i"};
    sc_core::sc_in<bool> ar_lock_i{"ar_lock_i"};
    sc_core::sc_in<sc_dt::sc_uint<4>> ar_cache_i{"ar_cache_i"};
    sc_core::sc_in<sc_dt::sc_uint<3>> ar_prot_i{"ar_prot_i"};
    sc_core::sc_in<sc_dt::sc_uint<4>> ar_qos_i{"ar_qos_i"};
    sc_core::sc_in<sc_dt::sc_uint<4>> ar_region_i{"ar_region_i"};
    sc_core::sc_in<bool> ar_valid_i{"ar_valid_i"};
    sc_core::sc_out<bool> ar_ready_o{"ar_ready_o"};

    // Read data channel signals
    sc_core::sc_out<sc_dt::sc_uint<IDWIDTH>> r_id_o{"r_id_o"};
    sc_core::sc_out<sc_dt::sc_biguint<BUSWIDTH>> r_data_o{"r_data_o"};
    sc_core::sc_out<sc_dt::sc_uint<2>> r_resp_o{"r_resp_o"};
    sc_core::sc_out<bool> r_last_o{"r_last_o"};
    sc_core::sc_out<bool> r_valid_o{"r_valid_o"};
    sc_core::sc_in<bool> r_ready_i{"r_ready_i"};

    tlm::tlm_sync_enum nb_transport_bw(payload_type& trans, phase_type& phase, sc_core::sc_time& t);

private:
    void bus_thread();

    /**
     * a handle class holding the pointer to the current transaction and associated phase
     */
    struct trans_handle {
        //! pointer to the associated AXITLM payload
    	payload_type* payload = nullptr;
    	//! current protocol phase
    	phase_type    phase = tlm::UNINITIALIZED_PHASE;
    };

    std::unordered_map<uint8_t, std::queue<std::shared_ptr<trans_handle>>> active_w_transactions;
    std::unordered_map<uint8_t, std::queue<std::shared_ptr<trans_handle>>> active_r_transactions;
	void register_trans(unsigned int axi_id, payload_type &trans, phase_type phase);
};


/////////////////////////////////////////////////////////////////////////////////////////
// Class definition
/////////////////////////////////////////////////////////////////////////////////////////
template <unsigned int BUSWIDTH, unsigned int ADDRWIDTH, unsigned int IDWIDTH>
inline axi_pin2tlm_adaptor<BUSWIDTH, ADDRWIDTH, IDWIDTH>::axi_pin2tlm_adaptor::axi_pin2tlm_adaptor(sc_core::sc_module_name nm)
: sc_module(nm) {
    output_socket.register_nb_transport_bw([this](payload_type& trans, phase_type& phase, sc_core::sc_time& t)
    		-> tlm::tlm_sync_enum { return nb_transport_bw(trans, phase, t); });

    SC_METHOD(bus_thread)
    sensitive << clk_i.pos() << resetn_i.neg();
}


template <unsigned int BUSWIDTH, unsigned int ADDRWIDTH, unsigned int IDWIDTH>
inline void axi_pin2tlm_adaptor<BUSWIDTH, ADDRWIDTH, IDWIDTH>::axi_pin2tlm_adaptor::bus_thread() {
    sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
    sc_dt::sc_biguint<BUSWIDTH> write_data{0};

	if(!resetn_i.read()) { // active-low reset
		SCCTRACE(SCMOD) << "Reset adapter";
	    r_valid_o.write(false);
	    r_last_o.write(false);
	    b_valid_o.write(false);
	    ar_ready_o.write(false);
	    aw_ready_o.write(false);
	} else {
		if(r_ready_i.read()) {
			r_valid_o.write(false);
			r_last_o.write(false);
		}
		if(b_ready_i.read())
			b_valid_o.write(false);

		w_ready_o.write(false);
		ar_ready_o.write(false);
		aw_ready_o.write(false);

		if (ar_valid_i.read()) {
			unsigned id = ar_id_i.read();

			payload_type* payload = tlm::tlm_mm<axi::axi_protocol_types>::get().allocate<axi::axi4_extension>();
		    auto ext = payload->get_extension<axi::axi4_extension>();
			auto addr = ar_addr_i.read();
			auto length   = ar_len_i.read();
			auto size     = 1 << ar_size_i.read();
			auto buf_size = size * (length+1);
			uint8_t* r_data_buf = new uint8_t[buf_size];

			payload->acquire();
			payload->set_address(addr);
			payload->set_data_length(buf_size);
			payload->set_streaming_width(buf_size);
			payload->set_command(tlm::TLM_READ_COMMAND);
			payload->set_data_ptr(r_data_buf);
		    ext->set_size(ar_size_i.read());
		    ext->set_length(length);
		    ext->set_burst(axi::into<axi::burst_e>(ar_burst_i.read().to_uint()));
		    ext->set_id(ar_id_i.read());
		    ext->set_exclusive(ar_lock_i.read());
		    ext->set_cache(ar_cache_i.read());
		    ext->set_prot(ar_prot_i.read());
		    ext->set_qos(ar_qos_i.read());
		    ext->set_region(ar_region_i.read());
		    ar_ready_o.write(true);
			phase_type phase = tlm::BEGIN_REQ;
			output_socket->nb_transport_fw(*payload, phase, delay);
			SCCTRACE(SCMOD) << phase << " of RD trans (axi_id:"<<id<<")";
			register_trans(id, *payload, phase);
		}

		// R channel
		for(auto& it: active_r_transactions) {
			auto read_trans = it.second.front();
			if (read_trans->phase == axi::BEGIN_PARTIAL_RESP || read_trans->phase  == tlm::BEGIN_RESP) { // send a single beat
			    sc_dt::sc_biguint<BUSWIDTH> read_beat{0};
				payload_type* p = read_trans->payload;
				auto ext = p->get_extension<axi::axi4_extension>();
			    sc_assert(ext && "axi4_extension missing");

			    read_trans->phase  = (read_trans->phase  == axi::BEGIN_PARTIAL_RESP) ? axi::END_PARTIAL_RESP : tlm::END_RESP;

				for(size_t i = 0, j = 0; j < ext->get_size(); i += 8, j++) {
					read_beat.range(i + 7, i) = *(uint8_t*)(p->get_data_ptr() + j);
				}

				auto id = ext->get_id();
				r_id_o.write(id);
				r_resp_o.write(axi::to_int(ext->get_resp()));
				r_data_o.write(read_beat);
				r_valid_o.write(true);

				p->set_address(p->get_address() + BUSWIDTH / 8);
				output_socket->nb_transport_fw(*p, read_trans->phase, delay);
				SCCTRACE(SCMOD) << read_trans->phase << " of RD trans (axi_id:"<<id<<")";


				// EDN_RESP indicates the last phase of the AXI Read transaction
				if(read_trans->phase == tlm::END_RESP) {
					r_last_o.write(true);
					p->release();
					auto it = active_r_transactions.find(id);
					if (it == active_r_transactions.end())
						SCCFATAL(SCMOD) << "Invalid read transaction ID " << id;
					auto trans_queue = it->second;
					trans_queue.pop();
					if (trans_queue.empty())
						active_r_transactions.erase(id);
				}
				break;
			}
		}

		if(aw_valid_i.read()) {
			unsigned id = aw_id_i.read();

			payload_type* trans = tlm::tlm_mm<axi::axi_protocol_types>::get().allocate<axi::axi4_extension>();
			register_trans(id, *trans, axi::BEGIN_PARTIAL_REQ);
			aw_ready_o.write(true);
		    auto ext = trans->get_extension<axi::axi4_extension>();
			auto length = aw_len_i.read();
			auto addr = aw_addr_i.read();
			auto num_bytes = 1 << aw_size_i.read();
			auto buf_size = num_bytes * (length+1);
			uint8_t* w_data_buf = new uint8_t[buf_size];


			trans->acquire();
			trans->set_address(addr);
			trans->set_data_length(buf_size);
			trans->set_streaming_width(buf_size);
			trans->set_command(tlm::TLM_WRITE_COMMAND);
			trans->set_data_ptr(w_data_buf);
		    ext->set_size(aw_size_i.read());
		    ext->set_length(length);
		    ext->set_burst(axi::into<axi::burst_e>(aw_burst_i.read().to_uint()));
		    ext->set_id(aw_id_i.read());
		    ext->set_exclusive(aw_lock_i.read());
		    ext->set_cache(aw_cache_i.read());
		    ext->set_prot(aw_prot_i.read());
		    ext->set_qos(aw_qos_i.read());
		    ext->set_region(aw_region_i.read());
		}

		if(w_valid_i.read()){
			unsigned id = aw_id_i.read();
			auto it = active_w_transactions.find(id);
			if (it == active_w_transactions.end())
				SCCERR(SCMOD) << "Invalid write transaction ID " << id;
			auto write_trans = it->second.front();
			payload_type* p = write_trans->payload;
		    auto ext = p->get_extension<axi::axi4_extension>();
		    sc_assert(ext && "axi4_extension missing");
			w_ready_o.write(true);
			write_data = w_data_i.read();
			auto num_bytes = 1 << aw_size_i.read();
			write_trans->payload->set_byte_enable_length(w_strb_i.read());
			auto data_ptr  = write_trans->payload->get_data_ptr();
			for (size_t i=0, j = 0; i < 8; j += num_bytes, i++) {
				data_ptr[i] = write_data.range(j + num_bytes - 1, j).to_uint64();
			}

			if (w_last_i.read()) {
				write_trans->phase = tlm::BEGIN_REQ;
			}
			output_socket->nb_transport_fw(*write_trans->payload, write_trans->phase, delay);
			SCCTRACE(SCMOD) << "FW: " << write_trans->phase << " of WR (axi_id:"<<id<<")";
		}

		// WR RESPONSE channel
		for(auto& it: active_w_transactions) {
			auto write_trans = it.second.front();
			if (write_trans->phase == tlm::BEGIN_RESP) {
				payload_type* p = write_trans->payload;
				auto ext = p->get_extension<axi::axi4_extension>();
			    sc_assert(ext && "axi4_extension missing");
			    auto id = ext->get_id();


			    write_trans->phase = tlm::END_RESP;
				b_valid_o.write(true);
				b_id_o.write(id);
				b_resp_o.write(axi::to_int(ext->get_resp()));
				output_socket->nb_transport_fw(*p, write_trans->phase, delay);
				SCCTRACE(SCMOD) << "FW: " << write_trans->phase << " of WR (axi_id:"<<id<<")";

				p->release();
				it.second.pop();
				if (it.second.empty())
					active_w_transactions.erase(id);

				break;
			}
		}
    }
}

template <unsigned int BUSWIDTH, unsigned int ADDRWIDTH, unsigned int IDWIDTH>
inline tlm::tlm_sync_enum axi_pin2tlm_adaptor<BUSWIDTH, ADDRWIDTH, IDWIDTH>::axi_pin2tlm_adaptor::nb_transport_bw(payload_type& trans, phase_type& phase, sc_core::sc_time& t) {
	auto id = axi::get_axi_id(trans);
	SCCTRACE(SCMOD) << "BW call: " << phase << " of "<<(trans.is_read()?"RD":"WR")<<" trans (axi_id:"<<id<<")";
	axi::axi4_extension* ext;
	trans.get_extension(ext);
	sc_assert(ext && "axi4_extension missing");
	tlm::tlm_sync_enum status{tlm::TLM_ACCEPTED};
	if(trans.is_read()){
		auto it = active_r_transactions.find(id);
		if (it == active_r_transactions.end())
			SCCERR(SCMOD) << "Invalid read transaction ID " << id;
		auto active_trans = it->second.front();
		active_trans->phase= phase;
	} else { // WRITE transaction
		auto it = active_w_transactions.find(id);
		if (it == active_w_transactions.end())
			SCCERR(SCMOD) << "Invalid write transaction ID " << id;
		auto active_trans = it->second.front();
		active_trans->phase= phase;
	}
	return status;
}

template<unsigned int BUSWIDTH, unsigned int ADDRWIDTH, unsigned int IDWIDTH>
void axi_pin2tlm_adaptor<BUSWIDTH, ADDRWIDTH, IDWIDTH>::register_trans(unsigned int axi_id, payload_type &trans, phase_type phase) {
	auto th = std::make_shared<trans_handle>();
	th->payload = &trans;
	th->phase = phase;
	if(trans.is_read())
		active_r_transactions[axi_id].push(th);
	else
		active_w_transactions[axi_id].push(th);
}

} // namespace axi_bfm
