/*
 * Copyright (c) 2010-2013 Qualcomm Atheros Communications Inc.
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

#ifndef REG_H
#define REG_H

/*
 * NOTE: Please DO NOT modify any code in this header file unless
 *       you know what you want to do.
 */

/*
 * Numbering from ISO 3166
 */
enum ath6kl_CountryCode {
	COUNTRY_ALBANIA             = 8,    /* Albania */
	COUNTRY_ALGERIA             = 12,   /* Algeria */
	COUNTRY_ARGENTINA           = 32,   /* Argentina */
	COUNTRY_ARMENIA             = 51,   /* Armenia */
	COUNTRY_ARUBA               = 533,  /* Aruba */
	COUNTRY_AUSTRALIA           = 36,   /* Australia (for STA) */
	COUNTRY_AUSTRALIA_AP        = 5000, /* Australia (for AP) */
	COUNTRY_AUSTRIA             = 40,   /* Austria */
	COUNTRY_AZERBAIJAN          = 31,   /* Azerbaijan */
	COUNTRY_BAHRAIN             = 48,   /* Bahrain */
	COUNTRY_BANGLADESH          = 50,   /* Bangladesh */
	COUNTRY_BARBADOS            = 52,   /* Barbados */
	COUNTRY_BELARUS             = 112,  /* Belarus */
	COUNTRY_BELGIUM             = 56,   /* Belgium */
	COUNTRY_BELIZE              = 84,   /* Belize */
	COUNTRY_BOLIVIA             = 68,   /* Bolivia */
	COUNTRY_BOSNIA_HERZEGOWANIA = 70,   /* Bosnia & Herzegowania */
	COUNTRY_BRAZIL              = 76,   /* Brazil */
	COUNTRY_BRUNEI_DARUSSALAM   = 96,   /* Brunei Darussalam */
	COUNTRY_BULGARIA            = 100,  /* Bulgaria */
	COUNTRY_CAMBODIA            = 116,  /* Cambodia */
	COUNTRY_CANADA              = 124,  /* Canada (for STA) */
	COUNTRY_CANADA_AP           = 5001, /* Canada (for AP) */
	COUNTRY_CHILE               = 152,  /* Chile */
	COUNTRY_CHINA               = 156,  /* People's Republic of China */
	COUNTRY_COLOMBIA            = 170,  /* Colombia */
	COUNTRY_COSTA_RICA          = 188,  /* Costa Rica */
	COUNTRY_CROATIA             = 191,  /* Croatia */
	COUNTRY_CYPRUS              = 196,
	COUNTRY_CZECH               = 203,  /* Czech Republic */
	COUNTRY_DENMARK             = 208,  /* Denmark */
	COUNTRY_DOMINICAN_REPUBLIC  = 214,  /* Dominican Republic */
	COUNTRY_ECUADOR             = 218,  /* Ecuador */
	COUNTRY_EGYPT               = 818,  /* Egypt */
	COUNTRY_EL_SALVADOR         = 222,  /* El Salvador */
	COUNTRY_ESTONIA             = 233,  /* Estonia */
	COUNTRY_FAEROE_ISLANDS      = 234,  /* Faeroe Islands */
	COUNTRY_FINLAND             = 246,  /* Finland */
	COUNTRY_FRANCE              = 250,  /* France */
	COUNTRY_FRANCE2             = 255,  /* France2 */
	COUNTRY_GEORGIA             = 268,  /* Georgia */
	COUNTRY_GERMANY             = 276,  /* Germany */
	COUNTRY_GREECE              = 300,  /* Greece */
	COUNTRY_GREENLAND           = 304,  /* Greenland */
	COUNTRY_GRENADA             = 308,  /* Grenada */
	COUNTRY_GUAM                = 316,  /* Guam */
	COUNTRY_GUATEMALA           = 320,  /* Guatemala */
	COUNTRY_HAITI               = 332,  /* Haiti */
	COUNTRY_HONDURAS            = 340,  /* Honduras */
	COUNTRY_HONG_KONG           = 344,  /* Hong Kong S.A.R., P.R.C. */
	COUNTRY_HUNGARY             = 348,  /* Hungary */
	COUNTRY_ICELAND             = 352,  /* Iceland */
	COUNTRY_INDIA               = 356,  /* India */
	COUNTRY_INDONESIA           = 360,  /* Indonesia */
	COUNTRY_IRAN                = 364,  /* Iran */
	COUNTRY_IRAQ                = 368,  /* Iraq */
	COUNTRY_IRELAND             = 372,  /* Ireland */
	COUNTRY_ISRAEL              = 376,  /* Israel */
	COUNTRY_ITALY               = 380,  /* Italy */
	COUNTRY_JAMAICA             = 388,  /* Jamaica */
	COUNTRY_JAPAN               = 392,  /* Japan */
	COUNTRY_JAPAN1              = 393,  /* Japan (JP1) */
	COUNTRY_JAPAN2              = 394,  /* Japan (JP0) */
	COUNTRY_JAPAN3              = 395,  /* Japan (JP1-1) */
	COUNTRY_JAPAN4              = 396,  /* Japan (JE1) */
	COUNTRY_JAPAN5              = 397,  /* Japan (JE2) */
	COUNTRY_JAPAN6              = 399,  /* Japan (JP6) */
	COUNTRY_JORDAN              = 400,  /* Jordan */
	COUNTRY_KAZAKHSTAN          = 398,  /* Kazakhstan */
	COUNTRY_KENYA               = 404,  /* Kenya */
	COUNTRY_KOREA_NORTH         = 408,  /* North Korea */
	COUNTRY_KOREA_ROC           = 410,  /* South Korea (for STA) */
	COUNTRY_KOREA_ROC2          = 411,  /* South Korea */
	COUNTRY_KOREA_ROC3          = 412,  /* South Korea (for AP) */
	COUNTRY_KUWAIT              = 414,  /* Kuwait */
	COUNTRY_LATVIA              = 428,  /* Latvia */
	COUNTRY_LEBANON             = 422,  /* Lebanon */
	COUNTRY_LIBYA               = 434,  /* Libya */
	COUNTRY_LIECHTENSTEIN       = 438,  /* Liechtenstein */
	COUNTRY_LITHUANIA           = 440,  /* Lithuania */
	COUNTRY_LUXEMBOURG          = 442,  /* Luxembourg */
	COUNTRY_MACAU               = 446,  /* Macau */
	COUNTRY_MACEDONIA           = 807,  /* Yugoslav Republic of Macedonia */
	COUNTRY_MALAYSIA            = 458,  /* Malaysia */
	COUNTRY_MALTA               = 470,  /* Malta */
	COUNTRY_MEXICO              = 484,  /* Mexico */
	COUNTRY_MONACO              = 492,  /* Principality of Monaco */
	COUNTRY_MOROCCO             = 504,  /* Morocco */
	COUNTRY_NEPAL               = 524,  /* Nepal */
	COUNTRY_NETHERLANDS         = 528,  /* Netherlands */
	COUNTRY_NETHERLAND_ANTILLES = 530,  /* Netherlands-Antilles */
	COUNTRY_NEW_ZEALAND         = 554,  /* New Zealand */
	COUNTRY_NICARAGUA           = 558,  /* Nicaragua */
	COUNTRY_NORWAY              = 578,  /* Norway */
	COUNTRY_OMAN                = 512,  /* Oman */
	COUNTRY_PAKISTAN            = 586,  /* Islamic Republic of Pakistan */
	COUNTRY_PANAMA              = 591,  /* Panama */
	COUNTRY_PARAGUAY            = 600,  /* Paraguay */
	COUNTRY_PERU                = 604,  /* Peru */
	COUNTRY_PHILIPPINES         = 608,  /* Republic of the Philippines */
	COUNTRY_POLAND              = 616,  /* Poland */
	COUNTRY_PORTUGAL            = 620,  /* Portugal */
	COUNTRY_PUERTO_RICO         = 630,  /* Puerto Rico */
	COUNTRY_QATAR               = 634,  /* Qatar */
	COUNTRY_ROMANIA             = 642,  /* Romania */
	COUNTRY_RUSSIA              = 643,  /* Russia */
	COUNTRY_RWANDA              = 646,  /* Rwanda */
	COUNTRY_SAUDI_ARABIA        = 682,  /* Saudi Arabia */
	COUNTRY_SERBIA              = 688,  /* Serbia */
	COUNTRY_MONTENEGRO          = 499,  /* Montenegro */
	COUNTRY_SINGAPORE           = 702,  /* Singapore */
	COUNTRY_SLOVAKIA            = 703,  /* Slovak Republic */
	COUNTRY_SLOVENIA            = 705,  /* Slovenia */
	COUNTRY_SOUTH_AFRICA        = 710,  /* South Africa */
	COUNTRY_SPAIN               = 724,  /* Spain */
	COUNTRY_SRILANKA            = 144,  /* Sri Lanka */
	COUNTRY_SWEDEN              = 752,  /* Sweden */
	COUNTRY_SWITZERLAND         = 756,  /* Switzerland */
	COUNTRY_SYRIA               = 760,  /* Syria */
	COUNTRY_TAIWAN              = 158,  /* Taiwan */
	COUNTRY_THAILAND            = 764,  /* Thailand */
	COUNTRY_TRINIDAD_Y_TOBAGO   = 780,  /* Trinidad y Tobago */
	COUNTRY_TUNISIA             = 788,  /* Tunisia */
	COUNTRY_TURKEY              = 792,  /* Turkey */
	COUNTRY_UAE                 = 784,  /* U.A.E. */
	COUNTRY_UGANDA              = 800,  /* Uganda */
	COUNTRY_UKRAINE             = 804,  /* Ukraine */
	COUNTRY_UNITED_KINGDOM      = 826,  /* United Kingdom */
	COUNTRY_UNITED_STATES       = 840,  /* United States (for STA) */
	COUNTRY_UNITED_STATES_AP    = 841,  /* United States (for AP) */
	COUNTRY_UNITED_STATES_PS    = 842,  /* United States - public safety */
	COUNTRY_URUGUAY             = 858,  /* Uruguay */
	COUNTRY_UZBEKISTAN          = 860,  /* Uzbekistan */
	COUNTRY_VENEZUELA           = 862,  /* Venezuela */
	COUNTRY_VIET_NAM            = 704,  /* Viet Nam */
	COUNTRY_YEMEN               = 887,  /* Yemen */
	COUNTRY_ZIMBABWE            = 716   /* Zimbabwe */
};

enum ath6kl_RegionCode {
	REGION_NO_ENUMRD   = 0x00,
	REGION_NULL1_WORLD = 0x03,     /* For 11b-only countries
					  (no 11a allowed) */
	REGION_NULL1_ETSIB = 0x07,     /* Israel */
	REGION_NULL1_ETSIC = 0x08,

	REGION_FCC1_FCCA   = 0x10,     /* USA */
	REGION_FCC1_WORLD  = 0x11,     /* Hong Kong */
	REGION_FCC2_FCCA   = 0x20,     /* Canada */
	REGION_FCC2_WORLD  = 0x21,     /* Australia & HK */
	REGION_FCC2_ETSIC  = 0x22,
	REGION_FCC3_FCCA   = 0x3A,     /* USA & Canada w/5470 band, 11h,
					  DFS enabled */
	REGION_FCC3_WORLD  = 0x3B,     /* USA & Canada w/5470 band, 11h,
					  DFS enabled */
	REGION_FCC3_ETSIC  = 0x3F,
	REGION_FCC4_FCCA   = 0x12,     /* FCC public safety plus UNII bands */
	REGION_FCC5_FCCA   = 0x13,     /* US with no DFS */
	REGION_FCC5_WORLD  = 0x16,     /* US with no DFS */
	REGION_FCC6_FCCA   = 0x14,     /* Same as FCC2_FCCA but with
					  5600-5650MHz channels disabled for
					  US & Canada APs */
	REGION_FCC6_WORLD  = 0x23,     /* Same as FCC2_FCCA but with
					  5600-5650MHz channels disabled for
					  Australia APs */

	REGION_ETSI1_WORLD = 0x37,

	REGION_ETSI2_WORLD = 0x35,     /* Hungary & others */
	REGION_ETSI3_WORLD = 0x36,     /* France & others */
	REGION_ETSI4_WORLD = 0x30,
	REGION_ETSI4_ETSIC = 0x38,
	REGION_ETSI5_WORLD = 0x39,
	REGION_ETSI6_WORLD = 0x34,     /* Bulgaria */
	REGION_ETSI8_WORLD = 0x3D,     /* Russia */
	REGION_ETSI9_WORLD = 0x3E,     /* Ukraine */
	REGION_ETSI_RESERVED   = 0x33,     /* Reserved (Do not used) */
	REGION_FRANCE_RES  = 0x31,     /* Legacy France for OEM */

	REGION_APL6_WORLD  = 0x5B,     /* Singapore */
	REGION_APL4_WORLD  = 0x42,     /* Singapore */
	REGION_APL3_FCCA   = 0x50,
	REGION_APL_RESERVED    = 0x44,     /* Reserved (Do not used)  */
	REGION_APL2_WORLD  = 0x45,     /* Korea */
	REGION_APL2_APLC   = 0x46,
	REGION_APL3_WORLD  = 0x47,
	REGION_APL2_APLD   = 0x49,     /* Korea with 2.3G channels */
	REGION_APL2_FCCA   = 0x4D,     /* Specific Mobile Customer */
	REGION_APL1_WORLD  = 0x52,     /* Latin America */
	REGION_APL1_FCCA   = 0x53,
	REGION_APL1_ETSIC  = 0x55,
	REGION_APL2_ETSIC  = 0x56,     /* Venezuela */
	REGION_APL5_WORLD  = 0x58,     /* Chile */
	REGION_APL7_FCCA   = 0x5C,
	REGION_APL8_WORLD  = 0x5D,
	REGION_APL9_WORLD  = 0x5E,
	REGION_APL10_WORLD = 0x5F,     /* Korea 5GHz for STA */


	REGION_MKK5_MKKA   = 0x99, /* This is a temporary value.
				      MG and DQ have to give official one */
	REGION_MKK5_FCCA   = 0x9A, /* This is a temporary value.
				      MG and DQ have to give official one */
	REGION_MKK5_MKKC   = 0x88,
	REGION_MKK11_MKKA  = 0xD4,
	REGION_MKK11_FCCA  = 0xD5,
	REGION_MKK11_MKKC  = 0xD7,

	/*
	* World mode SKUs
	*/
	REGION_WOR0_WORLD  = 0x60,     /* World0 (WO0 SKU) */
	REGION_WOR1_WORLD  = 0x61,     /* World1 (WO1 SKU) */
	REGION_WOR2_WORLD  = 0x62,     /* World2 (WO2 SKU) */
	REGION_WOR3_WORLD  = 0x63,     /* World3 (WO3 SKU) */
	REGION_WOR4_WORLD  = 0x64,     /* World4 (WO4 SKU) */
	REGION_WOR5_ETSIC  = 0x65,     /* World5 (WO5 SKU) */

	REGION_WOR01_WORLD = 0x66,     /* World0-1 (WW0-1 SKU) */
	REGION_WOR02_WORLD = 0x67,     /* World0-2 (WW0-2 SKU) */
	REGION_EU1_WORLD   = 0x68,     /* Same as World0-2 (WW0-2 SKU),
					  except active scan ch1-13. No ch14 */

	REGION_WOR9_WORLD  = 0x69,     /* World9 (WO9 SKU) */
	REGION_WORA_WORLD  = 0x6A,     /* WorldA (WOA SKU) */
	REGION_WORB_WORLD  = 0x6B,     /* WorldB (WOA SKU) */
	REGION_WORC_WORLD  = 0x6C,     /* WorldC (WOA SKU) */

	/*
	* Regulator domains ending in a number (e.g. APL1,
	* MK1, ETSI4, etc) apply to 5GHz channel and power
	* information.  Regulator domains ending in a letter
	* (e.g. APLA, FCCA, etc) apply to 2.4GHz channel and
	* power information.
	*/
	REGION_APL1        = 0x0150,   /* LAT & Asia */
	REGION_APL2        = 0x0250,   /* LAT & Asia */
	REGION_APL3        = 0x0350,   /* Taiwan */
	REGION_APL4        = 0x0450,   /* Jordan */
	REGION_APL5        = 0x0550,   /* Chile */
	REGION_APL6        = 0x0650,   /* Singapore */
	REGION_APL7        = 0x0750,   /* Taiwan */
	REGION_APL8        = 0x0850,   /* Malaysia */
	REGION_APL9        = 0x0950,   /* Korea */
	REGION_APL10       = 0x1050,   /* Korea 5GHz */

	REGION_ETSI1       = 0x0130,   /* Europe & others */
	REGION_ETSI2       = 0x0230,   /* Europe & others */
	REGION_ETSI3       = 0x0330,   /* Europe & others */
	REGION_ETSI4       = 0x0430,   /* Europe & others */
	REGION_ETSI5       = 0x0530,   /* Europe & others */
	REGION_ETSI6       = 0x0630,   /* Europe & others */
	REGION_ETSI8       = 0x0830,   /* Russia - only by APs */
	REGION_ETSI9       = 0x0930,   /* Ukraine - only by APs */
	REGION_ETSIB       = 0x0B30,   /* Israel */
	REGION_ETSIC       = 0x0C30,   /* Latin America */

	REGION_FCC1        = 0x0110,   /* US & others */
	REGION_FCC2        = 0x0120,   /* Canada, Australia & New Zealand */
	REGION_FCC3        = 0x0160,   /* US w/new middle band & DFS */
	REGION_FCC4        = 0x0165,
	REGION_FCC5        = 0x0180,
	REGION_FCC6        = 0x0610,
	REGION_FCCA        = 0x0A10,

	REGION_APLD        = 0x0D50,   /* South Korea */

	REGION_MKK1        = 0x0140,   /* Japan */
	REGION_MKK2        = 0x0240,   /* Japan Extended */
	REGION_MKK3        = 0x0340,   /* Japan new 5GHz */
	REGION_MKK4        = 0x0440,   /* Japan new 5GHz */
	REGION_MKK5        = 0x0540,   /* Japan new 5GHz */
	REGION_MKK6        = 0x0640,   /* Japan new 5GHz */
	REGION_MKK7        = 0x0740,   /* Japan new 5GHz */
	REGION_MKK8        = 0x0840,   /* Japan new 5GHz */
	REGION_MKK9        = 0x0940,   /* Japan new 5GHz */
	REGION_MKK10       = 0x1040,   /* Japan new 5GHz */
	REGION_MKK11       = 0x1140,   /* Japan new 5GHz */
	REGION_MKK12       = 0x1240,   /* Japan new 5GHz */

	REGION_MKKA        = 0x0A40,   /* Japan */
	REGION_MKKC        = 0x0A50,

	REGION_NULL1       = 0x0198,
	REGION_WORLD       = 0x0199,
	REGION_DEBUG_REG_DMN   = 0x01ff,
	REGION_UNINIT_REG_DMN  = 0x0fff,
};

#define ATH6KL_COUNTRY_ERD_FLAG		0x8000
#define ATH6KL_WORLDWIDE_ROAMING_FLAG	0x4000

#define ATH6KL_REG_CODE_MASK		(0xffff)

#define NULL_REG_CODE	(0xFFFF)

struct reg_code_to_isoname {
	u16 reg_code;
	char *iso_name;

	/*
	 * Map to CRDA's alpha2 and map to "00" for all super domains.
	 *
	 * "00" in cfg80211.ko or CRDA is world domain and the rules are
	 * CH1   - CH13  @ HT40
	 * CH12  - CH13  @ HT20, PASSIVE-SCAN | NO-IBSS
	 * CH14  - CH14  @ HT20, PASSIVE-SCAN | NO-IBSS | NO-OFDM
	 * CH36  - CH48  @ HT40, PASSIVE-SCAN | NO-IBSS
	 * CH149 - CH165 @ HT40, PASSIVE-SCAN | NO-IBSS
	 *
	 * This domain meet most ath6kl super domains except
	 * 1.no DFS channels support (5260 MHz - 5700 MHz)
	 * 2.all CH36 - CH48 of ath6kl treat as DFS
	 */
	char *reg_to_ctry_iso_name;
};

struct reg_info {
#define ATH6KL_REG_FALGS_INTERNAL_REGDB		(1 << 0)
#define ATH6KL_REG_FALGS_P2P_IN_PASV_CHAN	(1 << 1)
	u32 flags;
	struct ath6kl *ar;
	struct wiphy *wiphy;
	struct ieee80211_regdomain *current_regd;
	u32 current_reg_code;
};

extern const struct ieee80211_regdomain *ath6kl_reg_regdb_country[];
extern const struct ieee80211_regdomain *ath6kl_reg_regdb_region[];

int ath6kl_reg_notifier(struct wiphy *wiphy,
			struct regulatory_request *request);
void ath6kl_reg_notifier2(struct wiphy *wiphy,
			struct regulatory_request *request);
int ath6kl_reg_target_notify(struct ath6kl *ar, u32 reg_code);
bool ath6kl_reg_is_init_done(struct ath6kl *ar);
bool ath6kl_reg_is_p2p_channel(struct ath6kl *ar, u32 freq);
bool ath6kl_reg_is_dfs_channel(struct ath6kl *ar, u32 freq);
struct reg_info *ath6kl_reg_init(struct ath6kl *ar,
				bool intRegdb,
				bool p2pInPasvCh);
void ath6kl_reg_deinit(struct ath6kl *ar);
void ath6kl_reg_bss_info(struct ath6kl *ar,
			struct ieee80211_mgmt *mgmt,
			int len,
			u8 snr,
			struct ieee80211_channel *channel);
#endif /* REG_H */
