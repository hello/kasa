/*
 * Copyright (c) 2004-2013 Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "core.h"
#include "debug.h"

/* Default Regulatory for invalid reg_code. */
static struct ieee80211_regdomain ath6kl_regd_NA = {
	.n_reg_rules = 1,
	.alpha2 = "NA",
	.reg_rules = {
		REG_RULE(2412 - 10, 2462 + 10, 20, 3, 20, 0),
	}
};

/*
 * Naming from ISO 3166
 */
static const struct reg_code_to_isoname ath6kl_country_code_to_iso_name[] = {
	/* Country Code */
	{COUNTRY_ALBANIA,             "AL", NULL},
	{COUNTRY_ALGERIA,             "DZ", NULL},
	{COUNTRY_ARGENTINA,           "AR", NULL},
	{COUNTRY_ARMENIA,             "AM", NULL},
	{COUNTRY_ARUBA,               "AW", NULL},
	{COUNTRY_AUSTRALIA,           "AU", NULL},
	{COUNTRY_AUSTRALIA_AP,        "A2", NULL},
	{COUNTRY_AUSTRIA,             "AT", NULL},
	{COUNTRY_AZERBAIJAN,          "AZ", NULL},
	{COUNTRY_BAHRAIN,             "BH", NULL},
	{COUNTRY_BANGLADESH,          "BD", NULL},
	{COUNTRY_BARBADOS,            "BB", NULL},
	{COUNTRY_BELARUS,             "BY", NULL},
	{COUNTRY_BELGIUM,             "BE", NULL},
	{COUNTRY_BELIZE,              "BZ", NULL},
	{COUNTRY_BOLIVIA,             "BO", NULL},
	{COUNTRY_BOSNIA_HERZEGOWANIA, "BA", NULL},
	{COUNTRY_BRAZIL,              "BR", NULL},
	{COUNTRY_BRUNEI_DARUSSALAM,   "BN", NULL},
	{COUNTRY_BULGARIA,            "BG", NULL},
	{COUNTRY_CAMBODIA,            "KH", NULL},
	{COUNTRY_CANADA,              "CA", NULL},
	{COUNTRY_CANADA_AP,           "C2", NULL},
	{COUNTRY_CHILE,               "CL", NULL},
	{COUNTRY_CHINA,               "CN", NULL},
	{COUNTRY_COLOMBIA,            "CO", NULL},
	{COUNTRY_COSTA_RICA,          "CR", NULL},
	{COUNTRY_CROATIA,             "HR", NULL},
	{COUNTRY_CYPRUS,              "CY", NULL},
	{COUNTRY_CZECH,               "CZ", NULL},
	{COUNTRY_DENMARK,             "DK", NULL},
	{COUNTRY_DOMINICAN_REPUBLIC,  "DO", NULL},
	{COUNTRY_ECUADOR,             "EC", NULL},
	{COUNTRY_EGYPT,               "EG", NULL},
	{COUNTRY_EL_SALVADOR,         "SV", NULL},
	{COUNTRY_ESTONIA,             "EE", NULL},
	{COUNTRY_FAEROE_ISLANDS,      "FO", NULL},	/* NOT SUPPORT */
	{COUNTRY_FINLAND,             "FI", NULL},
	{COUNTRY_FRANCE,              "FR", NULL},
	{COUNTRY_FRANCE2,             "F2", NULL},
	{COUNTRY_GEORGIA,             "GE", NULL},
	{COUNTRY_GERMANY,             "DE", NULL},
	{COUNTRY_GREECE,              "GR", NULL},
	{COUNTRY_GREENLAND,           "GL", NULL},
	{COUNTRY_GRENADA,             "GD", NULL},
	{COUNTRY_GUAM,                "GU", NULL},
	{COUNTRY_GUATEMALA,           "GT", NULL},
	{COUNTRY_HAITI,               "HT", NULL},
	{COUNTRY_HONDURAS,            "HN", NULL},
	{COUNTRY_HONG_KONG,           "KH", NULL},
	{COUNTRY_HUNGARY,             "HU", NULL},
	{COUNTRY_ICELAND,             "IS", NULL},
	{COUNTRY_INDIA,               "IN", NULL},
	{COUNTRY_INDONESIA,           "ID", NULL},
	{COUNTRY_IRAN,                "IR", NULL},
	{COUNTRY_IRAQ,                "IQ", NULL},	/* NOT SUPPORT */
	{COUNTRY_IRELAND,             "IE", NULL},
	{COUNTRY_ISRAEL,              "IL", NULL},
	{COUNTRY_ITALY,               "IT", NULL},
	{COUNTRY_JAMAICA,             "JM", NULL},
	{COUNTRY_JAPAN,               "JP", NULL},
	{COUNTRY_JAPAN1,              "J2", NULL},	/* NOT SUPPORT */
	{COUNTRY_JAPAN2,              "J3", NULL},	/* NOT SUPPORT */
	{COUNTRY_JAPAN3,              "J4", NULL},	/* NOT SUPPORT */
	{COUNTRY_JAPAN4,              "J5", NULL},	/* NOT SUPPORT */
	{COUNTRY_JAPAN5,              "J6", NULL},	/* NOT SUPPORT */
	{COUNTRY_JAPAN6,              "J7", NULL},	/* NOT SUPPORT */
	{COUNTRY_JORDAN,              "JO", NULL},
	{COUNTRY_KAZAKHSTAN,          "KZ", NULL},
	{COUNTRY_KENYA,               "KE", NULL},
	{COUNTRY_KOREA_NORTH,         "KP", NULL},
	{COUNTRY_KOREA_ROC,           "KR", NULL},
	{COUNTRY_KOREA_ROC2,          "K2", NULL},
	{COUNTRY_KOREA_ROC3,          "K3", NULL},
	{COUNTRY_KUWAIT,              "KW", NULL},
	{COUNTRY_LATVIA,              "LV", NULL},
	{COUNTRY_LEBANON,             "LE", NULL},
	{COUNTRY_LIBYA,               "LY", NULL},	/* NOT SUPPORT */
	{COUNTRY_LIECHTENSTEIN,       "LI", NULL},
	{COUNTRY_LITHUANIA,           "LT", NULL},
	{COUNTRY_LUXEMBOURG,          "LU", NULL},
	{COUNTRY_MACAU,               "MO", NULL},
	{COUNTRY_MACEDONIA,           "MK", NULL},
	{COUNTRY_MALAYSIA,            "MY", NULL},
	{COUNTRY_MALTA,               "MT", NULL},
	{COUNTRY_MEXICO,              "MX", NULL},
	{COUNTRY_MONACO,              "MC", NULL},
	{COUNTRY_MOROCCO,             "MA", NULL},
	{COUNTRY_NEPAL,               "NP", NULL},
	{COUNTRY_NETHERLANDS,         "NL", NULL},
	{COUNTRY_NETHERLAND_ANTILLES, "AN", NULL},
	{COUNTRY_NEW_ZEALAND,         "NZ", NULL},
	{COUNTRY_NICARAGUA,           "NI", NULL},	/* NOT SUPPORT */
	{COUNTRY_NORWAY,              "NO", NULL},
	{COUNTRY_OMAN,                "OM", NULL},
	{COUNTRY_PAKISTAN,            "PK", NULL},
	{COUNTRY_PANAMA,              "PA", NULL},
	{COUNTRY_PARAGUAY,            "PY", NULL},	/* NOT SUPPORT */
	{COUNTRY_PERU,                "PE", NULL},
	{COUNTRY_PHILIPPINES,         "PH", NULL},
	{COUNTRY_POLAND,              "PL", NULL},
	{COUNTRY_PORTUGAL,            "PT", NULL},
	{COUNTRY_PUERTO_RICO,         "PR", NULL},
	{COUNTRY_QATAR,               "QA", NULL},
	{COUNTRY_ROMANIA,             "RO", NULL},
	{COUNTRY_RUSSIA,              "RU", NULL},
	{COUNTRY_RWANDA,              "RW", NULL},
	{COUNTRY_SAUDI_ARABIA,        "SA", NULL},
	{COUNTRY_SERBIA,              "RS", NULL},	/* NOT SUPPORT */
	{COUNTRY_MONTENEGRO,          "ME", NULL},
	{COUNTRY_SINGAPORE,           "SG", NULL},
	{COUNTRY_SLOVAKIA,            "SK", NULL},
	{COUNTRY_SLOVENIA,            "SI", NULL},
	{COUNTRY_SOUTH_AFRICA,        "ZA", NULL},
	{COUNTRY_SPAIN,               "ES", NULL},
	{COUNTRY_SRILANKA,            "LK", NULL},
	{COUNTRY_SWEDEN,              "SE", NULL},
	{COUNTRY_SWITZERLAND,         "CH", NULL},
	{COUNTRY_SYRIA,               "SY", NULL},
	{COUNTRY_TAIWAN,              "TW", NULL},
	{COUNTRY_THAILAND,            "TH", NULL},
	{COUNTRY_TRINIDAD_Y_TOBAGO,   "TT", NULL},
	{COUNTRY_TUNISIA,             "TN", NULL},
	{COUNTRY_TURKEY,              "TR", NULL},
	{COUNTRY_UAE,                 "AE", NULL},
	{COUNTRY_UGANDA,              "UG", NULL},
	{COUNTRY_UKRAINE,             "UA", NULL},
	{COUNTRY_UNITED_KINGDOM,      "GB", NULL},
	{COUNTRY_UNITED_STATES,       "US", NULL},
	{COUNTRY_UNITED_STATES_AP,    "U2", NULL},
	{COUNTRY_UNITED_STATES_PS,    "PS", NULL},
	{COUNTRY_URUGUAY,             "UY", NULL},
	{COUNTRY_UZBEKISTAN,          "UZ", NULL},
	{COUNTRY_VENEZUELA,           "VE", NULL},
	{COUNTRY_VIET_NAM,            "VN", NULL},
	{COUNTRY_YEMEN,               "YE", NULL},
	{COUNTRY_ZIMBABWE,            "ZW", NULL},
	/* keep last */
	{NULL_REG_CODE, NULL},
};

static const struct reg_code_to_isoname ath6kl_region_code_to_name[] = {
	/* Region Code */
	{REGION_NO_ENUMRD,    "00", NULL},	/* DEBUG */
	{REGION_NULL1_WORLD,  "03", "AL"},
	{REGION_NULL1_ETSIB,  "07", NULL},
	{REGION_NULL1_ETSIC,  "08", NULL},
	{REGION_FCC1_FCCA,    "10", "CO"},
	{REGION_FCC1_WORLD,   "11", "CR"},
	{REGION_FCC2_FCCA,    "20", NULL},
	{REGION_FCC2_WORLD,   "21", "BB"},
	{REGION_FCC2_ETSIC,   "22", NULL},
	{REGION_FCC3_FCCA,    "3a", "CA"},
	{REGION_FCC3_WORLD,   "3b", "AR"},
	{REGION_FCC3_ETSIC,   "3f", "NZ"},
	{REGION_FCC4_FCCA,    "12", "PS"},
	{REGION_FCC5_FCCA,    "13", NULL},
	{REGION_FCC5_WORLD,   "16", NULL},
	{REGION_FCC6_FCCA,    "14", "CA"},
	{REGION_FCC6_WORLD,   "23", "AU"},
	{REGION_ETSI1_WORLD,  "37", "AW"},
	{REGION_ETSI2_WORLD,  "35", "JO"},
	{REGION_ETSI3_WORLD,  "36", "CY"},
	{REGION_ETSI4_WORLD,  "30", "AM"},
	{REGION_ETSI4_ETSIC,  "38", NULL},	/* NOT SUPPORT */
	{REGION_ETSI5_WORLD,  "39", NULL},
	{REGION_ETSI6_WORLD,  "34", NULL},
	{REGION_ETSI8_WORLD,  "3d", "RU"},
	{REGION_ETSI9_WORLD,  "3e", "UA"},
	{REGION_ETSI_RESERVED, "33", NULL},	/* NOT SUPPORT */
	{REGION_FRANCE_RES,   "31", NULL},
	{REGION_APL6_WORLD,   "5b", "BH"},
	{REGION_APL4_WORLD,   "42", "MA"},
	{REGION_APL3_FCCA,    "50", NULL},
	{REGION_APL_RESERVED, "44", NULL},	/* NOT SUPPORT */
	{REGION_APL2_WORLD,   "45", "ID"},
	{REGION_APL2_APLC,    "46", NULL},	/* NOT SUPPORT */
	{REGION_APL3_WORLD,   "47", NULL},
	{REGION_APL2_APLD,    "49", "KR"},
	{REGION_APL2_FCCA,    "4d", NULL},
	{REGION_APL1_WORLD,   "52", "CN"},
	{REGION_APL1_FCCA,    "53", NULL},	/* NOT SUPPORT */
	{REGION_APL1_ETSIC,   "55", "BZ"},
	{REGION_APL2_ETSIC,   "56", NULL},
	{REGION_APL5_WORLD,   "58", NULL},
	{REGION_APL7_FCCA,    "5c", "TW"},
	{REGION_APL8_WORLD,   "5d", NULL},
	{REGION_APL9_WORLD,   "5e", "KP"},
	{REGION_APL10_WORLD,  "5f", "KR"},
	{REGION_MKK5_MKKA,    "99", NULL},
	{REGION_MKK5_FCCA,    "9a", NULL},
	{REGION_MKK5_MKKC,    "88", "JP"},
	{REGION_MKK11_MKKA,   "d4", NULL},
	{REGION_MKK11_FCCA,   "d5", NULL},
	{REGION_MKK11_MKKC,   "d7", NULL},
	{REGION_WOR0_WORLD,   "60", "00"},
	{REGION_WOR1_WORLD,   "61", "00"},
	{REGION_WOR2_WORLD,   "62", "00"},
	{REGION_WOR3_WORLD,   "63", "00"},
	{REGION_WOR4_WORLD,   "64", "00"},
	{REGION_WOR5_ETSIC,   "65", "00"},
	{REGION_WOR01_WORLD,  "66", "00"},
	{REGION_WOR02_WORLD,  "67", "00"},
	{REGION_EU1_WORLD,    "68", "00"},
	{REGION_WOR9_WORLD,   "69", "00"},
	{REGION_WORA_WORLD,   "6a", "00"},	/* Default region code */
	{REGION_WORB_WORLD,   "6b", "00"},
	{REGION_WORC_WORLD,   "6c", "00"},

	/* keep last */
	{NULL_REG_CODE, NULL},
};

static inline bool __is_ht40_not_allowed(struct ieee80211_channel *chan)
{
	if (!chan)
		return true;

	/* HT40 is not allowed in CH12, CH13 and CH14. */
	if ((chan->center_freq == 2467) ||
		(chan->center_freq == 2472) ||
		(chan->center_freq == 2484))
		return true;

	if (chan->flags & IEEE80211_CHAN_DISABLED)
		return true;

	if (IEEE80211_CHAN_NO_HT40 == (chan->flags & (IEEE80211_CHAN_NO_HT40)))
		return true;

	return false;
}

static void _reg_process_chan(struct ieee80211_supported_band *sband,
				int chan_idx)
{
	struct ieee80211_channel *channel;
	struct ieee80211_channel *channel_before = NULL, *channel_after = NULL;
	unsigned int i;
	u16 start_freq = 0;

	channel = &sband->channels[chan_idx];

	if (__is_ht40_not_allowed(channel)) {
		channel->flags |= IEEE80211_CHAN_NO_HT40;
		return;
	}

	for (i = 0; i < sband->n_channels; i++) {
		struct ieee80211_channel *c = &sband->channels[i];

		if (c->center_freq == (channel->center_freq - 20))
			channel_before = c;
		if (c->center_freq == (channel->center_freq + 20))
			channel_after = c;
	}

	/* Only update it the channel still not yet marked as HT40+/HT40-.*/
	if (channel->center_freq < 4000) {
		if (!(channel->flags & IEEE80211_CHAN_NO_HT40MINUS)) {
			if (__is_ht40_not_allowed(channel_before))
				channel->flags |= IEEE80211_CHAN_NO_HT40MINUS;
			else
				channel->flags &= ~IEEE80211_CHAN_NO_HT40MINUS;
		}

		if (!(channel->flags & IEEE80211_CHAN_NO_HT40PLUS)) {
			if (__is_ht40_not_allowed(channel_after))
				channel->flags |= IEEE80211_CHAN_NO_HT40PLUS;
			else
				channel->flags &= ~IEEE80211_CHAN_NO_HT40PLUS;
		}
	} else { /* No overlap HT40 in 5G. */
		/*
		 * Don't care 5170(CH34) - 5230(CH46) because no HT40
		 * channel in it.
		 */
		if (channel->center_freq >= 5745)
			start_freq = 5745;	/* Upper U-NII */
		else
			start_freq = 5180;

		if (((channel->center_freq - start_freq) % 40) == 0) {
			/* HT40+ only channel */
			channel->flags |= IEEE80211_CHAN_NO_HT40MINUS;

			if (!(channel->flags & IEEE80211_CHAN_NO_HT40PLUS)) {
				if (__is_ht40_not_allowed(channel_after))
					channel->flags |=
						IEEE80211_CHAN_NO_HT40PLUS;
				else
					channel->flags &=
						~IEEE80211_CHAN_NO_HT40PLUS;
			}
		} else {
			/* HT40- only channel */
			channel->flags |= IEEE80211_CHAN_NO_HT40PLUS;

			if (!(channel->flags & IEEE80211_CHAN_NO_HT40MINUS)) {
				if (__is_ht40_not_allowed(channel_before))
					channel->flags |=
						IEEE80211_CHAN_NO_HT40MINUS;
				else
					channel->flags &=
						~IEEE80211_CHAN_NO_HT40MINUS;
			}
		}
	}

	return;
}

static void ath6kl_reg_chan_flags_update(struct reg_info *reg)
{
	struct wiphy *wiphy = reg->wiphy;
	struct ieee80211_supported_band *sband;
	enum ieee80211_band band;
	int i;

	for (band = 0; band < IEEE80211_NUM_BANDS; band++) {
		if (wiphy->bands[band]) {
			sband = wiphy->bands[band];
			for (i = 0; i < sband->n_channels; i++)
				_reg_process_chan(sband, i);
		}
	}

	return;
}

static const char *_reg_find_iso_name(u16 reg_code,
					bool isCountry,
					bool reg2Ctry)
{
	const struct reg_code_to_isoname *name;

	if (isCountry)
		reg2Ctry = false;

	if (isCountry)
		name = &ath6kl_country_code_to_iso_name[0];
	else
		name = &ath6kl_region_code_to_name[0];
	while (name) {
		if ((name->reg_code == reg_code) &&
			(name->iso_name)) {
			if (reg2Ctry)
				return name->reg_to_ctry_iso_name;
			else
				return name->iso_name;
		}

		name++;
	}

	ath6kl_err("reg iosname search fail! code is 0x%x\n", reg_code);

	return NULL;
}

static struct ieee80211_regdomain *_reg_find_regd(u16 code,
						bool isCountry)
{
	struct ieee80211_regdomain *regd;
	struct ieee80211_regdomain **reg_regdb;
	const char *alpha2 = _reg_find_iso_name(code, isCountry, false);

	if (alpha2 == NULL)
		return NULL;

	if (isCountry)
		reg_regdb = (struct ieee80211_regdomain **)
						ath6kl_reg_regdb_country;
	else
		reg_regdb = (struct ieee80211_regdomain **)
						ath6kl_reg_regdb_region;

	while (*reg_regdb) {
		regd = *reg_regdb;
		if ((regd->alpha2[0] == alpha2[0]) &&
			(regd->alpha2[1] == alpha2[1]))
			return regd;

		reg_regdb++;
	}

	ath6kl_err("reg regd search fail! code is 0x%x\n", code);

	return NULL;
}

static struct ieee80211_regdomain *ath6kl_reg_get_regd(struct reg_info *reg,
							u32 reg_code)
{
	struct ieee80211_regdomain *regd = NULL;
	u16 code, code_type;

	/*
	 * If it's a country code and search country code list first,
	 * then region domain list. And, assume it must be a valid code.
	 * If return is NULL and back to use the default's.
	 */
	code = (u16)(reg_code & ATH6KL_REG_CODE_MASK);
	code_type = (u16)(reg_code >> ATH6KL_COUNTRY_RD_SHIFT);
	if (code_type & ATH6KL_COUNTRY_ERD_FLAG)
		regd = _reg_find_regd(code, true);
	else
		regd = _reg_find_regd(code, false);

	if (regd == NULL) {
		regd = &ath6kl_regd_NA;

		ath6kl_err("reg get regd fail, using default NA.\n");
	}

	ath6kl_dbg(ATH6KL_DBG_REGDB,
			"reg code 0x%0x, %s code, WWR %s --> %c%c, %d rules\n",
			reg_code,
			(code_type & ATH6KL_COUNTRY_ERD_FLAG) ?
					"Country" : "Region",
			(code_type & ATH6KL_WORLDWIDE_ROAMING_FLAG) ?
					"ON" : "OFF",
			regd->alpha2[0], regd->alpha2[1],
			regd->n_reg_rules);

	return regd;
}

static int __reg_freq_reg_info_regd(u32 center_freq,
			      u32 desired_bw_khz,
			      const struct ieee80211_reg_rule **reg_rule,
			      const struct ieee80211_regdomain *custom_regd,
			      bool *is_freq_start,
			      bool *is_freq_end)
{
#define ONE_GHZ_IN_KHZ	1000000
	int i;
	bool band_rule_found = false;
	bool bw_fits = false;
	u32 start_freq_khz, end_freq_khz;

	if (!desired_bw_khz)
		desired_bw_khz = MHZ_TO_KHZ(20);

	if (!custom_regd)
		return -EINVAL;

	for (i = 0; i < custom_regd->n_reg_rules; i++) {
		const struct ieee80211_reg_rule *rr;
		const struct ieee80211_freq_range *fr = NULL;

		rr = &custom_regd->reg_rules[i];
		fr = &rr->freq_range;

		if (!band_rule_found) {
			if (abs(center_freq - fr->start_freq_khz) <=
				(2 * ONE_GHZ_IN_KHZ))
				band_rule_found = true;
			if (abs(center_freq - fr->end_freq_khz) <=
				(2 * ONE_GHZ_IN_KHZ))
				band_rule_found = true;
		}

		start_freq_khz = center_freq - (desired_bw_khz/2);
		end_freq_khz = center_freq + (desired_bw_khz/2);

		if (band_rule_found &&
		    fr->start_freq_khz > MHZ_TO_KHZ(4000) &&
		    start_freq_khz >= fr->start_freq_khz &&
		    end_freq_khz <= fr->end_freq_khz &&
		    ((center_freq - fr->start_freq_khz) % MHZ_TO_KHZ(20)) == 0)
			return -ERANGE;

		if (start_freq_khz >= fr->start_freq_khz &&
		    end_freq_khz <= fr->end_freq_khz)
			bw_fits = true;
		else
			bw_fits = false;

		if (band_rule_found && bw_fits) {
			if (center_freq > MHZ_TO_KHZ(4000)) {
				if (start_freq_khz == fr->start_freq_khz)
					*is_freq_start = true;
				if (end_freq_khz == fr->end_freq_khz)
					*is_freq_end = true;
			}
			*reg_rule = rr;
			return 0;
		}
	}

	if (!band_rule_found)
		return -ERANGE;

	return -EINVAL;
#undef ONE_GHZ_IN_KHZ
}

static void _reg_handle_channel(struct ieee80211_channel *chan,
				const struct ieee80211_regdomain *regd)
{
	int r;
	u32 desired_bw_khz = MHZ_TO_KHZ(20);
	u32 bw_flags = 0;
	u32 channel_flags = 0;
	const struct ieee80211_reg_rule *reg_rule = NULL;
	const struct ieee80211_power_rule *power_rule = NULL;
	const struct ieee80211_freq_range *freq_range = NULL;
	bool is_freq_start = false, is_freq_end = false;

	r = __reg_freq_reg_info_regd(MHZ_TO_KHZ(chan->center_freq),
					desired_bw_khz,
					&reg_rule,
					regd,
					&is_freq_start,
					&is_freq_end);

	if (r) {
		chan->flags =
		chan->orig_flags = IEEE80211_CHAN_DISABLED;
		return;
	}

	power_rule = &reg_rule->power_rule;
	freq_range = &reg_rule->freq_range;

	if (freq_range->max_bandwidth_khz < MHZ_TO_KHZ(40))
		bw_flags = IEEE80211_CHAN_NO_HT40;
	else {
		/*
		 * A hint to let _reg_process_chan() know that this channel
		 * is a boundry channel and HT40+/HT40- already known.
		 */
		if (is_freq_start)
			bw_flags |= IEEE80211_CHAN_NO_HT40MINUS;

		if (is_freq_end)
			bw_flags |= IEEE80211_CHAN_NO_HT40PLUS;
	}

	if (reg_rule->flags & NL80211_RRF_PASSIVE_SCAN)
		channel_flags |= IEEE80211_CHAN_PASSIVE_SCAN;
	if (reg_rule->flags & NL80211_RRF_NO_IBSS)
		channel_flags |= IEEE80211_CHAN_NO_IBSS;
	if (reg_rule->flags & NL80211_RRF_DFS)
		channel_flags |= IEEE80211_CHAN_RADAR;

	chan->flags = (channel_flags | bw_flags);
	chan->orig_flags = chan->flags;
	chan->max_antenna_gain = (int) MBI_TO_DBI(power_rule->max_antenna_gain);
	chan->max_power = (int) MBM_TO_DBM(power_rule->max_eirp);

	return;
}

static void ath6kl_reg_apply_regulatory(struct reg_info *reg,
				   const struct ieee80211_regdomain *regd)
{
	struct ieee80211_supported_band *sband;
	struct ieee80211_channel *chan;
	enum ieee80211_band band;
	int i;

	for (band = 0; band < IEEE80211_NUM_BANDS; band++) {
		if (!reg->wiphy->bands[band])
			continue;

		sband = reg->wiphy->bands[band];
		for (i = 0; i < sband->n_channels; i++) {
			chan = &sband->channels[i];
			_reg_handle_channel(chan, regd);

			/*
			 * If this channel is P2P allowed and not marked
			 * as PASSIVE/IBSS to let wpa_supplicant use it.
			 */
			if ((reg->flags & ATH6KL_REG_FALGS_P2P_IN_PASV_CHAN) &&
			    (!(chan->flags & IEEE80211_CHAN_RADAR)) &&
			    ath6kl_p2p_is_p2p_channel(chan->center_freq)) {
				chan->flags &= ~IEEE80211_CHAN_PASSIVE_SCAN;
				chan->flags &= ~IEEE80211_CHAN_NO_IBSS;
			}
		}
	}
}

static void _reg_update_report(struct wiphy *wiphy, const char *alpha2)
{
	char remap_alpha2[2];
	static u32 flip;

#ifdef CONFIG_ATH6KL_REGDB_AS_CFG80211_REGDB
	/*
	 * If cfg80211-internal-regdb uses our regdb.c and just pass
	 * our alpha2 code to cfg80211.
	 */
	remap_alpha2[0] = alpha2[0];
	remap_alpha2[1] = alpha2[1];
#else
	/*
	 * HACK: Here just kick the different alpha2 to force to trigger
	 *       regdb update to user.
	 */
	if (flip++ & 0x1) {
		remap_alpha2[0] = 'U';
		remap_alpha2[1] = 'S';
	} else {
		remap_alpha2[0] = '0';
		remap_alpha2[1] = '0';
	}
#endif

	/* Update to cfg80211 or CRDA */
	regulatory_hint(wiphy, remap_alpha2);

	return;
}

static struct ieee80211_regdomain *ath6kl_reg_update(struct reg_info *reg,
							u32 reg_code,
							bool target_update)
{
	struct ieee80211_regdomain *regd;

	BUG_ON(!reg);

	ath6kl_dbg(ATH6KL_DBG_REGDB,
				"reg update reg_code %x %sfrom target\n",
				reg_code,
				target_update ? "" : "not ");


	/* Query the local regulatory database from alpha2 words. */
	regd = ath6kl_reg_get_regd(reg, reg_code);

	/* HACK: Fetch local regulatory database to support channel list. */
	ath6kl_reg_apply_regulatory(reg, regd);

	/* Update the channel flags. */
	ath6kl_reg_chan_flags_update(reg);

	if (target_update && regd)
		_reg_update_report(reg->wiphy, regd->alpha2);

	/* Notify to update the channel record. */
	ath6kl_p2p_rc_fetch_chan(reg->ar);

	return regd;
}

static void ath6kl_reg_cfg80211_update(struct reg_info *reg,
					u32 reg_code,
					bool target_update)
{
	u16 code;
	const char *iso_name;
	char alpha2[2];

	BUG_ON(!reg);

	ath6kl_dbg(ATH6KL_DBG_REGDB,
			"reg cfg80211_update reg_code %x %sfrom target\n",
			reg_code,
			target_update ? "" : "not ");

	code = (u16)(reg_code & ATH6KL_REG_CODE_MASK);
	if ((reg_code >> ATH6KL_COUNTRY_RD_SHIFT) &
						ATH6KL_COUNTRY_ERD_FLAG)
		iso_name = _reg_find_iso_name(reg_code, true, false);
	else
		iso_name = _reg_find_iso_name(reg_code, false, true);

	if (iso_name) {
		alpha2[0] = iso_name[0];
		alpha2[1] = iso_name[1];

		ath6kl_dbg(ATH6KL_DBG_REGDB,
				"Alpha2 string being used: %c%c\n",
				alpha2[0], alpha2[1]);

		/* Update to cfg80211 & CRDA */
		regulatory_hint(reg->wiphy, alpha2);
	}

	/* Notify to update the channel record. */
	ath6kl_p2p_rc_fetch_chan(reg->ar);

	return;
}

static int _reg_cfg80211_notify(struct wiphy *wiphy,
					struct regulatory_request *request)
{
	char initiatorString[4][16] = {
		"driver",
		"core",
		"user",
		"country-ie",
	};
	struct ath6kl *ar = wiphy_priv(wiphy);
	int ret = 0;

	ath6kl_dbg(ATH6KL_DBG_REGDB,
		"cfg cfg80211_notify %c%c%s%s initiator %s\n",
		request->alpha2[0], request->alpha2[1],
		request->intersect ? " intersect" : "",
		request->processed ? " processed" : "",
		initiatorString[request->initiator]);

	ret = ath6kl_reg_set_country(ar, request->alpha2);

	return ret;
}

static int _reg_notifier(struct wiphy *wiphy,
			struct regulatory_request *request)
{
	char initiatorString[4][16] = {
		"driver",
		"core",
		"user",
		"country-ie",
	};
	struct ath6kl *ar = (struct ath6kl *)wiphy_priv(wiphy);
	struct reg_info *reg = ar->reg_ctx;
	struct ieee80211_regdomain *regd = NULL;

	switch (request->initiator) {
	case NL80211_REGDOM_SET_BY_DRIVER:
	case NL80211_REGDOM_SET_BY_CORE:
	case NL80211_REGDOM_SET_BY_USER:
	case NL80211_REGDOM_SET_BY_COUNTRY_IE:
		ath6kl_dbg(ATH6KL_DBG_REGDB,
					"reg notify by %s %c%c\n",
					initiatorString[request->initiator],
					request->alpha2[0],
					request->alpha2[1]);

		/* HACK: always use ours */
		regd = ath6kl_reg_update(reg, reg->current_reg_code, false);
		if (regd != reg->current_regd) {
			ath6kl_err("reg notifier fail, %x %p %p\n",
					reg->current_reg_code,
					reg->current_regd,
					regd);
			WARN_ON(1);
		}
		break;
	}

	return 0;
}

int ath6kl_reg_notifier(struct wiphy *wiphy,
			struct regulatory_request *request)
{
	struct ath6kl *ar = (struct ath6kl *)wiphy_priv(wiphy);
	struct reg_info *reg = ar->reg_ctx;

	if (reg == NULL)
		return 0;

	if (reg->flags & ATH6KL_REG_FALGS_INTERNAL_REGDB)
		_reg_notifier(wiphy, request);
	else if (reg->flags & ATH6KL_REG_FALGS_CFG80211_REGDB)
		_reg_cfg80211_notify(wiphy, request);

	return 0;
}

void ath6kl_reg_notifier2(struct wiphy *wiphy,
			struct regulatory_request *request)
{
	ath6kl_reg_notifier(wiphy, request);

	return;
}

int ath6kl_reg_target_notify(struct ath6kl *ar, u32 reg_code)
{
	struct reg_info *reg = ar->reg_ctx;
	struct ieee80211_regdomain *regd = NULL;

	BUG_ON(!reg);

	if ((reg_code & ATH6KL_REG_CODE_MASK) == NULL_REG_CODE) {
		ath6kl_err("reg unknown code 0x%x, ignore it\n", reg_code);

		/* Looks set country was rejected by the target. */
		if (((reg->flags & ATH6KL_REG_FALGS_INTERNAL_REGDB) ||
		     (reg->flags & ATH6KL_REG_FALGS_CFG80211_REGDB)) &&
		    test_bit(REG_COUNTRY_UPDATE, &ar->flag)) {
			clear_bit(REG_COUNTRY_UPDATE, &ar->flag);
			wake_up(&ar->event_wq);
		}

		return -EINVAL;
	}

	if (reg->flags & ATH6KL_REG_FALGS_INTERNAL_REGDB)
		regd = ath6kl_reg_update(reg, reg_code, true);
	else if (reg->flags & ATH6KL_REG_FALGS_CFG80211_REGDB)
		ath6kl_reg_cfg80211_update(reg, reg_code, true);
	else {
		u16 code;
		const char *iso_name;
		char alpha2[2];

		code = (u16)(reg_code & ATH6KL_REG_CODE_MASK);
		if ((reg_code >> ATH6KL_COUNTRY_RD_SHIFT) &
							ATH6KL_COUNTRY_ERD_FLAG)
			iso_name = _reg_find_iso_name(reg_code, true, false);
		else
			iso_name = _reg_find_iso_name(reg_code, false, true);

		if (iso_name) {
			alpha2[0] = iso_name[0];
			alpha2[1] = iso_name[1];

			ath6kl_dbg(ATH6KL_DBG_REGDB,
					"Country alpha2 being used: %c%c\n",
					alpha2[0], alpha2[1]);

			/* Update to cfg80211 & CRDA */
			regulatory_hint(ar->wiphy, alpha2);
		}
	}

	/* Cache the latest regulatory */
	reg->current_regd = regd;
	reg->current_reg_code = reg_code;

	return 0;
}

bool ath6kl_reg_is_init_done(struct ath6kl *ar)
{
	struct reg_info *reg = ar->reg_ctx;

	if (!((reg->current_reg_code >> ATH6KL_COUNTRY_RD_SHIFT) &
						ATH6KL_WORLDWIDE_ROAMING_FLAG))
		return true;
	else
		return false;
}

bool ath6kl_reg_is_p2p_channel(struct ath6kl *ar, u32 freq)
{
#define _REG_CHAN_P2P_NOT_ALLOWED (				\
				IEEE80211_CHAN_RADAR |		\
				IEEE80211_CHAN_PASSIVE_SCAN |	\
				IEEE80211_CHAN_NO_IBSS)
	if (ath6kl_p2p_is_p2p_channel(freq)) {
		struct wiphy *wiphy = ar->wiphy;
		enum ieee80211_band band = NL80211_BAND_2GHZ;
		struct ieee80211_supported_band *sband;
		struct ieee80211_channel *chan;
		int i;

		if (freq > 4000)
			band = NL80211_BAND_5GHZ;

		sband = wiphy->bands[band];
		for (i = 0; i < sband->n_channels; i++) {
			chan = &sband->channels[i];
			if (chan->center_freq == freq) {
				if (chan->flags & _REG_CHAN_P2P_NOT_ALLOWED)
					return false;
				else
					return true;
			}
		}
		return false;
	} else
		return false;
#undef _REG_CHAN_P2P_NOT_ALLOWED
}

bool ath6kl_reg_is_dfs_channel(struct ath6kl *ar, u32 freq)
{
	struct wiphy *wiphy = ar->wiphy;
	struct ieee80211_supported_band *sband;
	struct ieee80211_channel *chan;
	int i;

	if (freq < 4000)
		return false;

	sband = wiphy->bands[NL80211_BAND_5GHZ];
	for (i = 0; i < sband->n_channels; i++) {
		chan = &sband->channels[i];
		if (chan->center_freq == freq) {
			if (chan->flags & IEEE80211_CHAN_RADAR)
				return true;
			else
				return false;
		}
	}

	return false;
}

bool ath6kl_reg_is_lte_channel(struct ath6kl *ar, u32 freq)
{
#define _LTE_BAND7_START_FREQ	(2457)	/* Not used after Ch10 */

	/*
	 * FDD LTE band 7 uplink (2500Mhz ~ 2570MHz)
	 * TDD LTE band 40 (2300Mhz ~ 2400MHz)
	 */

	if (freq >= _LTE_BAND7_START_FREQ)
		return true;
	else
		return false;
#undef _LTE_BAND7_START_FREQ
}

struct reg_info *ath6kl_reg_init(struct ath6kl *ar,
				bool intRegdb,
				bool cfgRegdb,
				bool p2pInPasvCh)
{
	struct reg_info *reg;

	reg = kzalloc(sizeof(struct reg_info), GFP_KERNEL);
	if (!reg) {
		ath6kl_err("failed to alloc memory for reg\n");
		return NULL;
	}

	BUG_ON((intRegdb && cfgRegdb));

	reg->ar = ar;
	reg->wiphy = ar->wiphy;
	if (intRegdb) {
		reg->flags |= ATH6KL_REG_FALGS_INTERNAL_REGDB;
		if (p2pInPasvCh)
			reg->flags |= ATH6KL_REG_FALGS_P2P_IN_PASV_CHAN;

		ath6kl_info("Using driver's regdb%s.\n",
				(p2pInPasvCh ? " & p2p-in-passive-chan" : ""));
	} else if (cfgRegdb) {
		reg->flags |= ATH6KL_REG_FALGS_CFG80211_REGDB;

		ath6kl_info("Using cfg80211's regdb.\n");
	}

	ath6kl_dbg(ATH6KL_DBG_REGDB,
		   "reg init flags %x\n",
		   reg->flags);

	return reg;
}

void ath6kl_reg_deinit(struct ath6kl *ar)
{
	kfree(ar->reg_ctx);
	ar->reg_ctx = NULL;

	ath6kl_dbg(ATH6KL_DBG_REGDB,
		   "reg deinit");

	return;
}

static bool _reg_dump_country_ie(u8 *ies, int ies_len, u8 *alpha2)
{
	u8 *pos;
	bool found = false;

	pos = ies;
	if (ies && ies_len) {
		while (pos + 1 < ies + ies_len) {
			if (pos + 2 + pos[1] > ies + ies_len)
				break;
			if (pos[0] == WLAN_EID_COUNTRY) {
				found = true;
				memcpy(alpha2, (pos + 2), 2);
				break;
			}
			pos += 2 + pos[1];
		}
	}

	return found;
}

void ath6kl_reg_bss_info(struct ath6kl *ar,
			struct ieee80211_mgmt *mgmt,
			int len,
			u8 snr,
			struct ieee80211_channel *channel)
{
	if (!ath6kl_reg_is_init_done(ar)) {
		char alpha[2];
		bool found = false;

		if ((mgmt->frame_control == IEEE80211_STYPE_BEACON) ||
		    (mgmt->frame_control == IEEE80211_STYPE_PROBE_RESP))
			found = _reg_dump_country_ie(mgmt->u.beacon.variable,
							(len - 36),
							(u8 *)alpha);

		ath6kl_dbg(ATH6KL_DBG_REGDB,
			   "reg bssinfo BSSID %02x:%02x:%02x:%02x:%02x:%02x "
			   "fc %02x freq %d snr %3d %c%c\n",
			   mgmt->bssid[0], mgmt->bssid[1], mgmt->bssid[2],
			   mgmt->bssid[3], mgmt->bssid[4], mgmt->bssid[5],
			   mgmt->frame_control,
			   (channel ? channel->center_freq : 0),
			   snr,
			   (found ? alpha[0] : ' '),
			   (found ? alpha[1] : ' '));
	}

	return;
}

static void _reg_set_country(struct ath6kl *ar)
{
	struct ath6kl_vif *vif;
	/* These 11 channels are always active channel */
	u16 ch_list[11] = {2412, 2417, 2422, 2427,
			   2432, 2437, 2442, 2447,
			   2452, 2457, 2462};
	bool scan_on_going = false;
	int i;

	/* Any scan on-going? */
	for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);
		if (vif && vif->scan_req)
			scan_on_going = true;
	}

	/* Start a quick scan to kick it works */
	if (scan_on_going == false)
		ath6kl_wmi_startscan_cmd(ar->wmi,
					0, WMI_LONG_SCAN,
					true, false, 0, 0,
					11,
					ch_list);

	ath6kl_dbg(ATH6KL_DBG_REGDB,
		   "reg set country done, scan_on_going %d\n",
		   scan_on_going);

	return;
}

int ath6kl_reg_set_country(struct ath6kl *ar, char *isoName)
{
#define WAIT_REG_RESULT		(HZ / 5)	/* 200 ms. */
	struct ath6kl_vif *vif;
	struct reg_info *reg = ar->reg_ctx;
	long left;
	int i;

	BUG_ON(!reg);

	if (!(reg->flags & (ATH6KL_REG_FALGS_INTERNAL_REGDB |
				ATH6KL_REG_FALGS_CFG80211_REGDB)))
		return -EPERM;

	for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);
		if ((vif) &&
		    (test_bit(CONNECTED, &vif->flags) ||
		     test_bit(CONNECT_PEND, &vif->flags))) {
			ath6kl_err("reg not allow to change now\n");

			return -EPERM;
		}
	}

	if (down_interruptible(&ar->sem))
		return -EBUSY;

	set_bit(REG_COUNTRY_UPDATE, &ar->flag);

	ath6kl_dbg(ATH6KL_DBG_REGDB,
		   "reg set country %c%c\n",
		   isoName[0], isoName[1]);

	if (ath6kl_wmi_set_regdomain_cmd(ar->wmi, isoName)) {
		clear_bit(REG_COUNTRY_UPDATE, &ar->flag);
		up(&ar->sem);

		return -EIO;
	}

	left = wait_event_interruptible_timeout(ar->event_wq,
						!test_bit(REG_COUNTRY_UPDATE,
							  &ar->flag),
						WAIT_REG_RESULT);
	up(&ar->sem);

	if (test_bit(REG_COUNTRY_UPDATE, &ar->flag)) {
		clear_bit(REG_COUNTRY_UPDATE, &ar->flag);
		_reg_set_country(ar);
	} else {
		ath6kl_err("reg set country failed\n");

		return -EINVAL;
	}

	return 0;
#undef WAIT_REG_RESULT
}

#define AR6004_BOARD_DATA_ADDR		0x00400854
#define AR6004_BOARD_DATA_OFFSET	4
#define AR6004_RD_OFFSET		20

#define AR6006_BOARD_DATA_ADDR		0x00428854
#define AR6006_BOARD_DATA_OFFSET	4
#define AR6006_RD_OFFSET		20

int ath6kl_reg_set_rdcode(struct ath6kl *ar, unsigned short rdcode)
{
	u8 buf[32];
	u16 o_sum, o_ver, o_rd, o_rd_next;
	u32 n_rd, n_sum;
	u32 bd_addr = 0;
	int ret;
	u32 rd_offset, bd_offset;

	/* TODO: check rdcode invalid or not? */
	if (rdcode == NULL_REG_CODE)
		return -EINVAL;

	switch (ar->target_type) {
	case TARGET_TYPE_AR6004:
		rd_offset = AR6004_RD_OFFSET;
		bd_offset = AR6004_BOARD_DATA_OFFSET;
		ret = ath6kl_bmi_read(ar,
					AR6004_BOARD_DATA_ADDR,
					(u8 *)&bd_addr,
					4);
		break;
	case TARGET_TYPE_AR6006:
		rd_offset = AR6006_RD_OFFSET;
		bd_offset = AR6006_BOARD_DATA_OFFSET;
		ret = ath6kl_bmi_read(ar,
					AR6006_BOARD_DATA_ADDR,
					(u8 *)&bd_addr,
					4);
		break;
	default:
		ath6kl_err("No support rdcode overwrite! target_type %d\n",
				ar->target_type);
		return -EOPNOTSUPP;
	}

	if (ret)
		return ret;

	memset(buf, 0, sizeof(buf));
	ret = ath6kl_bmi_read(ar, bd_addr, buf, sizeof(buf));
	if (ret)
		return ret;

	memcpy((u8 *)&o_sum, buf + bd_offset, 2);
	memcpy((u8 *)&o_ver, buf + bd_offset + 2, 2);
	memcpy((u8 *)&o_rd, buf + rd_offset, 2);
	memcpy((u8 *)&o_rd_next, buf + rd_offset + 2, 2);

	ath6kl_dbg(ATH6KL_DBG_REGDB,
		   "reg set rd_code 0x%x ver 0x%x ori 0x%x-%x\n",
		   rdcode,
		   o_ver,
		   o_rd_next,
		   o_rd);

	n_rd = (o_rd_next << 16) + rdcode;
	ret = ath6kl_bmi_write(ar,
				bd_addr + rd_offset,
				(u8 *)&n_rd,
				4);
	if (ret)
		return ret;

	n_sum = (o_ver << 16) + (o_sum ^ o_rd ^ rdcode);
	ret = ath6kl_bmi_write(ar,
				bd_addr + bd_offset,
				(u8 *)&n_sum,
				4);

	return ret;
}

