// license:BSD-3-Clause
// copyright-holders:Olivier Galibert
/*********************************************************************

    Implementation of the Apple SWIM2 floppy disk controller

*********************************************************************/

#include "emu.h"
#include "swim2.h"

DEFINE_DEVICE_TYPE(SWIM2, swim2_device, "swim2", "Apple SWIM2 (Sander/Wozniak Integrated Machine) version 2 floppy controller")

swim2_device::swim2_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock) :
	applefdc_device(mconfig, SWIM2, tag, owner, clock)
{
}

void swim2_device::device_start()
{
	applefdc_device::device_start();

	save_item(NAME(m_mode));
	save_item(NAME(m_setup));
	save_item(NAME(m_param_idx));
	save_item(NAME(m_param));
	save_item(NAME(m_last_sync));
}

void swim2_device::device_reset()
{
	applefdc_device::device_reset();

	m_mode = 0x40;
	m_setup = 0x00;
	m_param_idx = 0;
	memset(m_param, 0, sizeof(m_param));
	m_floppy = nullptr;
	
	m_devsel_cb(0);
	m_sel35_cb(true);
	m_hdsel_cb(false);
}

void swim2_device::device_timer(emu_timer &, device_timer_id, int, void *)
{
}

void swim2_device::set_floppy(floppy_image_device *floppy)
{
	if(m_floppy == floppy)
		return;

	sync();
	flush_write();

	if(m_floppy)
		m_floppy->mon_w(true);
	m_floppy = floppy;
	if(m_mode & 0x80)
		m_floppy->mon_w(false);
	update_phases();
}

floppy_image_device *swim2_device::get_floppy() const
{
	return m_floppy;
}

void swim2_device::flush_write()
{
	if(m_floppy && m_last_sync > m_flux_write_start) {
		if(m_flux_write_count && m_flux_write[m_flux_write_count-1] == m_last_sync)
			m_flux_write_count--;
		attotime start = cycles_to_time(m_flux_write_start);
		attotime end = cycles_to_time(m_last_sync);
		std::vector<attotime> fluxes(m_flux_write_count);
		for(u32 i=0; i != m_flux_write_count; i++)
			fluxes[i] = cycles_to_time(m_flux_write[i]);
		m_floppy->write_flux(start, end, m_flux_write_count, m_flux_write_count ? &fluxes[0] : nullptr);
	}
	m_flux_write_count = 0;
	m_flux_write_start = m_last_sync;
}

void swim2_device::show_mode() const
{
	logerror("mode%s %s hdsel=%c %c%s %c%c%s\n",
			 m_mode & 0x80 ? " motoron" : "",
			 m_mode & 0x40 ? "ism" : "iwm",
			 m_mode & 0x20 ? '1' : '0',
			 m_mode & 0x10 ? 'w' : 'r',
			 m_mode & 0x08 ? " action" : "",
			 m_mode & 0x04 ? 'a' : '-',
			 m_mode & 0x02 ? 'b' : '-',
			 m_mode & 0x01 ? " clear" : "");

}

u8 swim2_device::read(offs_t offset)
{
	sync();

	static const char *const names[] = {
		"?0", "?1", "?2", "?3", "?4", "?5", "?6", "?7",
		"data", "mark", "crc", "param", "phases", "setup", "status", "handshake"
	};
	switch(offset) {
	case 0x3: case 0xb: {
		u8 r = m_param[m_param_idx];
		m_param_idx = (m_param_idx + 1) & 15;
		return r;
	}		
	case 0x4: case 0xc:
		return m_phases;
	case 0x5: case 0xd:
		return m_setup;
	case 0xe:
		return m_mode;
	default:
		logerror("read %s\n", names[offset & 15]);
		break;
	}
	return 0xff;
}

void swim2_device::write(offs_t offset, u8 data)
{
	sync();

	static const char *const names[] = {
		"data", "mark", "crc", "param", "phases", "setup", "mode0", "mode1",
		"?8", "?9", "?a", "?b", "?c", "?d", "?e", "?f"
	};

	switch(offset) {
	case 0x3: case 0xb: {
#if 0
		static const char *const pname[16] = {
			"minct", "mult", "ssl", "sss", "sll", "sls", "rpt", "csls",
			"lsl", "lss", "lll", "lls", "late", "time0", "early", "time1"
		};
#endif
		static const char *const pname[4] = {
			"late", "time0", "early", "time1"
		};
		logerror("param[%s] = %02x\n", pname[m_param_idx], data);
		m_param[m_param_idx] = data;
		m_param_idx = (m_param_idx + 1) & 3;
		break;
	}		
	case 0x4: {
		m_phases = data;
		update_phases();
		break;
	}

	case 0x5: case 0xd:
		m_setup = data;
		m_sel35_cb(m_setup & 0x02);
#if 0
		logerror("setup timer=%s tsm=%s %s ecm=%s %s %s 3.5=%s %s\n",
				 m_setup & 0x80 ? "on" : "off",
				 m_setup & 0x40 ? "off" : "on",
				 m_setup & 0x20 ? "ibm" : "apple",
				 m_setup & 0x10 ? "on" : "off",
				 m_setup & 0x08 ? "fclk/2" : "fclk",
				 m_setup & 0x04 ? "gcr" : "mfm",
				 m_setup & 0x02 ? "off" : "on",
				 m_setup & 0x01 ? "hdsel" : "q3");
#endif
		logerror("setup write=%s %s test=%s %s %s 3.5=%s %s\n",
				 m_setup & 0x40 ? "gcr" : "mfm",
				 m_setup & 0x20 ? "ibm" : "apple",
				 m_setup & 0x10 ? "on" : "off",
				 m_setup & 0x08 ? "fclk/2" : "fclk",
				 m_setup & 0x04 ? "gcr" : "mfm",
				 m_setup & 0x02 ? "off" : "on",
				 m_setup & 0x01 ? "wrinvert" : "wrdirect");
		break;

	case 0x6:
		m_mode &= ~data;
		m_mode |= 0x40;
		m_param_idx = 0;
		show_mode();
		break;

	case 0x7:
		//		if(!(m_mode & 0x01) && (data & 0x01))
		//			fifo_clear();
		m_mode |= data;
		show_mode();
		break;

	default:
		logerror("write %s, %02x\n", names[offset], data);
		break;
	}

	
	if((m_mode & 0x18) == 0x18) {
		// In write mode
		if(!m_flux_write_start) {
			m_flux_write_count = 0;
			m_flux_write_start = m_last_sync;
			logerror("write start\n");
		}
	} else {
		// Not in write mode
		if(m_flux_write_start) {
			flush_write();
			m_flux_write_start = 0;
			logerror("write end\n");
		}
	}
}

u64 swim2_device::time_to_cycles(const attotime &tm) const
{
	return tm.as_ticks(clock());
}

attotime swim2_device::cycles_to_time(u64 cycles) const
{
	return attotime::from_ticks(cycles, clock());
}

void swim2_device::sync()
{
	u64 next_sync = machine().time().as_ticks(clock());
	if(!(m_mode & 0x08)) {
		m_last_sync = next_sync;
		return;
	}

	logerror("ACTIVE %s %d-%d (%d)\n", m_mode & 0x10 ? "write" : "read", m_last_sync, next_sync, next_sync - m_last_sync);
	m_last_sync = next_sync;
}
