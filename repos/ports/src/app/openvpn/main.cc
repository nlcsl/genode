/**
 * \brief  TUN/TAP to Nic_session interface
 * \author Josef Soentgen
 * \date   2014-06-05
 */

/*
 * Copyright (C) 2014-2015 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU General Public License version 2.
 */

/* Genode includes */
#include <os/static_root.h>
#include <nic/component.h>
#include <root/component.h>
#include <libc/component.h>
#include <libc/allocator.h>
#include <base/heap.h>
#include <base/attached_rom_dataspace.h>

/* libc includes */
#include <unistd.h>

/* local includes */
#include "tuntap.h"


static int const verbose = false;


/*********************************
 ** OpenVPN main thread wrapper **
 *********************************/

extern "C" int openvpn_main(int, char*[]);


class Openvpn
{
	private:

		Genode::Semaphore _startup;

		struct Arg {
			int    c;
			char **v;
		} _arg;

		Arg _extract_arg(Libc::Env &env, Genode::Allocator &alloc) const
		{
			Arg arg;

			env.config([&] (Genode::Xml_node config) {

				arg.c = 0;
				config.for_each_sub_node("arg", [&] (Genode::Xml_node const &node) {
					/* check if the 'value' attribute exists */
					if (node.has_attribute("value"))
						++arg.c;
				});

				arg.v = new (alloc) char*[arg.c];

				int arg_i = 0;

				config.for_each_sub_node( "arg", [&] (Genode::Xml_node const &node) {
					/* insert an argument */
					try {
						Genode::Xml_attribute attr = node.attribute("value");

						Genode::size_t const arg_len = attr.value_size() + 1;
						char *a = arg.v[arg_i] = new (alloc) char[arg_len];

						attr.value(a, arg_len);
						++arg_i;

					} catch (Genode::Xml_node::Nonexistent_attribute) { }
				});
			});

			return arg;
		}

	public:

		Openvpn(Libc::Env &env, Genode::Allocator &alloc)
		: _arg(_extract_arg(env, alloc)) { }

		~Openvpn()
		{
//			for (int i = 0; i < argc; i++)
//				Genode::destroy( _alloc, argv[i] );
//
//			Genode::destroy( _alloc, argv );
		}

		void start() { _startup.up(); }

		int run()
		{
			_startup.down();

			return Libc::with_libc([&] () {
				return ::openvpn_main(_arg.c, _arg.v);
			});
		}
};


static Tuntap_device* _tuntap_dev;


Tuntap_device *tuntap_dev()
{
	return _tuntap_dev;
}


/***************************************
 ** Implementation of the Nic service **
 ***************************************/

class Openvpn_component : public Tuntap_device,
                          public Nic::Session_component
{
	private:
		Genode::Env &_env;
		Genode::Entrypoint &_ep;

		char const *_packet;

		enum { READ = 0, WRITE = 1 };

		int _pipefd[2];
		Genode::Semaphore _startup_lock;
		Genode::Semaphore _tx_lock;

	protected:

		bool _send()
		{
			//Genode::error("Openvpn_component::_send");
			using namespace Genode;

			if (!_tx.sink()->ready_to_ack())
				return false;

			if (!_tx.sink()->packet_avail())
				return false;

			Packet_descriptor packet = _tx.sink()->get_packet();
			//if (!packet.valid()) {
			// 	Genode::warning("Invalid tx packet");
			// 	return true;
			//}

			_packet = _tx.sink()->packet_content(packet);

			/* notify openvpn */
			::write(_pipefd[WRITE], "1", 1);

			/* block while openvpn handles the packet */
			_tx_lock.down();
			_tx.sink()->acknowledge_packet(packet);

			return true;
		}

		void _handle_packet_stream() override
		{
			while (_rx.source()->ack_avail())
				_rx.source()->release_packet(_rx.source()->get_acked_packet());

			while (_send()) ;
		}

	public:

		Openvpn_component( Genode::Env &env, Genode::Entrypoint &ep,
				           Genode::size_t const tx_buf_size,
		                   Genode::size_t const rx_buf_size,
		                   Genode::Allocator   &rx_block_md_alloc)
			: Session_component(tx_buf_size, rx_buf_size, rx_block_md_alloc, env, ep),
			  _env(env), _ep(ep)
		{
			if ( pipe( _pipefd ) )
			{
				Genode::error( "could not create pipe" );
				throw Genode::Exception();
			}
		}

		/**************************************
		 ** Nic::Session_component interface **
		 **************************************/

		Nic::Mac_address mac_address() override
		{
			static const char ADDR[] = { 0x40, 0x00, 0x00, 0x00, 0x00, 0x00 };
			return Nic::Mac_address( ( void * )ADDR );
		}

		bool link_state() override
		{
			/* XXX always return true for now */
			return true;
		}

		/***********************
		 ** TUN/TAP interface **
		 ***********************/

		int fd() { return _pipefd[READ]; }

		/* tx */
		int read(char *buf, Genode::size_t len)
		{
			Genode::memcpy(buf, _packet, len);
			_packet = 0;

			/* unblock nic client */
			_tx_lock.up();

			return len;
		}

		/* rx */
		int write(char const *buf, Genode::size_t len)
		{
			//_handle_packet_stream();

			if (!_rx.source()->ready_to_submit())
				return 0;

			try {
				Genode::Packet_descriptor packet = _rx.source()->alloc_packet(len);
				Genode::memcpy(_rx.source()->packet_content(packet), buf, len);
				_rx.source()->submit_packet(packet);
			} catch (...) { return 0; }

			return len;
		}

		void up() { 
			Genode::error("tuntap::up()");
			_startup_lock.up(); 
		}

		void down() { 
			Genode::error("tuntap::down()");
			_startup_lock.down(); 
		}
};


class Root : public Genode::Root_component<Openvpn_component, Genode::Single_client>
{
	private:

		Genode::Env &_env;
		Genode::Entrypoint &_ep;
		Genode::Allocator &_alloc;

		Openvpn &_openvpn;

	protected:

		Openvpn_component *_create_session(const char *args)
		{
			using namespace Genode;

			size_t ram_quota   = Arg_string::find_arg(args, "ram_quota"  ).ulong_value(0);
			size_t tx_buf_size = Arg_string::find_arg(args, "tx_buf_size").ulong_value(0);
			size_t rx_buf_size = Arg_string::find_arg(args, "rx_buf_size").ulong_value(0);

			/* deplete ram quota by the memory needed for the session structure */
			size_t session_size = max(4096UL, (unsigned long)sizeof(Openvpn_component));
			if (ram_quota < session_size)
				throw Ram_transfer::Quota_exceeded();

			/*
			 * Check if donated ram quota suffices for both communication
			 * buffers. Also check both sizes separately to handle a
			 * possible overflow of the sum of both sizes.
			 */
			if ( tx_buf_size               > ram_quota - session_size
			        || rx_buf_size               > ram_quota - session_size
			        || tx_buf_size + rx_buf_size > ram_quota - session_size )
			{
				Genode::error( "insufficient 'ram_quota', got %zd, need %zd",
				               ram_quota, tx_buf_size + rx_buf_size + session_size );
				throw Ram_transfer::Quota_exceeded();
			}

			Openvpn_component *component = new(Root::md_alloc())
				Openvpn_component(_env, _ep, tx_buf_size, rx_buf_size, _alloc);

			/**
			 * Setting the pointer in this manner is quite hackish but it has
			 * to be valid before OpenVPN calls open_tun(), which unfortunatly
			 * is early.
			 */
			_tuntap_dev = component;

			_openvpn.start();

			/* wait until OpenVPN configured the TUN/TAP device for the first time */
			_tuntap_dev->down();

			return component;
		}

		void _destroy_session(Openvpn_component *session)
		{
			Genode::destroy( Root::md_alloc(), session );
//			_openvpn.stop();
		}

	public:

		Root(Genode::Env &env, Genode::Entrypoint &ep, Genode::Allocator &md_alloc,
		     Openvpn &openvpn)
		:
			Genode::Root_component<Openvpn_component, Genode::Single_client>(ep, md_alloc),
			_env(env), _ep(ep), _alloc(md_alloc), _openvpn(openvpn)
		{ }
};


struct Server
{
	Libc::Env       &_env;
	Libc::Allocator  _heap;

	Openvpn _openvpn { _env, _heap };

	Genode::Entrypoint _ep { _env, 64*1024, "server_ep" };

	::Root _nic_root;
	
	Server(Libc::Env &env) : _env(env), _nic_root(_env, _ep, _heap, _openvpn)
	{
		_env.parent().announce(_ep.manage(_nic_root));
	}

	int run_openvpn() { return _openvpn.run(); }
};

void Libc::Component::construct(Libc::Env &env)
{
	static Server server(env);

	env.parent().exit(server.run_openvpn());
}
