// license:GPL-2.0+
// copyright-holders:Jarek Burczynski, Tomasz Slanina
#include "sound/msm5232.h"
#include "machine/gen_latch.h"
#include "machine/taito68705interface.h"
#include "emupal.h"
#include "screen.h"

class bigevglf_state : public driver_device
{
public:
	bigevglf_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_paletteram(*this, "paletteram"),
		m_spriteram1(*this, "spriteram1"),
		m_spriteram2(*this, "spriteram2"),
		m_audiocpu(*this, "audiocpu"),
		m_bmcu(*this, "bmcu"),
		m_maincpu(*this, "maincpu"),
		m_msm(*this, "msm"),
		m_soundlatch(*this, "soundlatch%u", 1),
		m_gfxdecode(*this, "gfxdecode"),
		m_screen(*this, "screen"),
		m_palette(*this, "palette"),
		m_debug_input(*this, "DEBUG")

		{ }

	/* memory pointers */
	required_shared_ptr<uint8_t> m_paletteram;
	required_shared_ptr<uint8_t> m_spriteram1;
	required_shared_ptr<uint8_t> m_spriteram2;

	/* video-related */
	bitmap_ind16 m_tmp_bitmap[4];
	std::unique_ptr<uint8_t[]>    m_vidram;
	uint32_t   m_vidram_bank;
	uint32_t   m_plane_selected;
	uint32_t   m_plane_visible;

	/* MCU related */
	int      m_mcu_coin_bit5;

	/* misc */
	uint32_t   m_beg_bank;
	uint8_t    m_beg13_ls74[2];
	uint8_t    m_port_select;     /* for muxed controls */

	/* devices */
	required_device<cpu_device> m_audiocpu;
	optional_device<taito68705_mcu_device> m_bmcu;
	void beg_banking_w(uint8_t data);
	uint8_t soundstate_r();
	void beg13_a_clr_w(uint8_t data);
	void beg13_b_clr_w(uint8_t data);
	void beg13_a_set_w(uint8_t data);
	void beg13_b_set_w(uint8_t data);
	uint8_t beg_status_r();
	uint8_t beg_trackball_x_r();
	uint8_t beg_trackball_y_r();
	void beg_port08_w(uint8_t data);
	uint8_t sub_cpu_mcu_coin_port_r();
	void bigevglf_palette_w(offs_t offset, uint8_t data);
	void bigevglf_gfxcontrol_w(uint8_t data);
	void bigevglf_vidram_addr_w(uint8_t data);
	void bigevglf_vidram_w(offs_t offset, uint8_t data);
	uint8_t bigevglf_vidram_r(offs_t offset);
	void init_bigevglf();
	virtual void machine_start() override;
	virtual void machine_reset() override;
	virtual void video_start() override;
	uint32_t screen_update_bigevglf(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	TIMER_CALLBACK_MEMBER(deferred_ls74_w);
	void draw_sprites( bitmap_ind16 &bitmap, const rectangle &cliprect );
	required_device<cpu_device> m_maincpu;
	required_device<msm5232_device> m_msm;
	required_device_array<generic_latch_8_device, 2> m_soundlatch;
	required_device<gfxdecode_device> m_gfxdecode;
	required_device<screen_device> m_screen;
	required_device<palette_device> m_palette;
	void bigevglf(machine_config &config);
	void bigevglf_portmap(address_map &map);
	void bigevglf_sub_portmap(address_map &map);
	void main_map(address_map &map);
	void sound_map(address_map &map);
	void sub_map(address_map &map);

	required_ioport m_debug_input;
	DECLARE_CUSTOM_INPUT_MEMBER( debug_in_r );
	void soundlatch_write(uint8_t data);
};
