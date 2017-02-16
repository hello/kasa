IF :LNOT: :DEF: _SI_REG_S
_SI_REG_S EQU 1

SI_CONFIG_ADDRESS                        EQU 0x0c00c000
SI_CONFIG_ERR_INT_MSB                    EQU 19
SI_CONFIG_ERR_INT_LSB                    EQU 19
SI_CONFIG_ERR_INT_MASK                   EQU 0x00080000
SI_CONFIG_BIDIR_OD_DATA_MSB              EQU 18
SI_CONFIG_BIDIR_OD_DATA_LSB              EQU 18
SI_CONFIG_BIDIR_OD_DATA_MASK             EQU 0x00040000
SI_CONFIG_I2C_MSB                        EQU 16
SI_CONFIG_I2C_LSB                        EQU 16
SI_CONFIG_I2C_MASK                       EQU 0x00010000
SI_CONFIG_POS_SAMPLE_MSB                 EQU 7
SI_CONFIG_POS_SAMPLE_LSB                 EQU 7
SI_CONFIG_POS_SAMPLE_MASK                EQU 0x00000080
SI_CONFIG_POS_DRIVE_MSB                  EQU 6
SI_CONFIG_POS_DRIVE_LSB                  EQU 6
SI_CONFIG_POS_DRIVE_MASK                 EQU 0x00000040
SI_CONFIG_INACTIVE_DATA_MSB              EQU 5
SI_CONFIG_INACTIVE_DATA_LSB              EQU 5
SI_CONFIG_INACTIVE_DATA_MASK             EQU 0x00000020
SI_CONFIG_INACTIVE_CLK_MSB               EQU 4
SI_CONFIG_INACTIVE_CLK_LSB               EQU 4
SI_CONFIG_INACTIVE_CLK_MASK              EQU 0x00000010
SI_CONFIG_DIVIDER_MSB                    EQU 2
SI_CONFIG_DIVIDER_LSB                    EQU 0
SI_CONFIG_DIVIDER_MASK                   EQU 0x00000007

SI_CS_ADDRESS                            EQU 0x0c00c004
SI_CS_BIT_CNT_IN_LAST_BYTE_MSB           EQU 13
SI_CS_BIT_CNT_IN_LAST_BYTE_LSB           EQU 11
SI_CS_BIT_CNT_IN_LAST_BYTE_MASK          EQU 0x00003800
SI_CS_DONE_ERR_MSB                       EQU 10
SI_CS_DONE_ERR_LSB                       EQU 10
SI_CS_DONE_ERR_MASK                      EQU 0x00000400
SI_CS_DONE_INT_MSB                       EQU 9
SI_CS_DONE_INT_LSB                       EQU 9
SI_CS_DONE_INT_MASK                      EQU 0x00000200
SI_CS_START_MSB                          EQU 8
SI_CS_START_LSB                          EQU 8
SI_CS_START_MASK                         EQU 0x00000100
SI_CS_RX_CNT_MSB                         EQU 7
SI_CS_RX_CNT_LSB                         EQU 4
SI_CS_RX_CNT_MASK                        EQU 0x000000f0
SI_CS_TX_CNT_MSB                         EQU 3
SI_CS_TX_CNT_LSB                         EQU 0
SI_CS_TX_CNT_MASK                        EQU 0x0000000f

SI_TX_DATA0_ADDRESS                      EQU 0x0c00c008
SI_TX_DATA0_DATA3_MSB                    EQU 31
SI_TX_DATA0_DATA3_LSB                    EQU 24
SI_TX_DATA0_DATA3_MASK                   EQU 0xff000000
SI_TX_DATA0_DATA2_MSB                    EQU 23
SI_TX_DATA0_DATA2_LSB                    EQU 16
SI_TX_DATA0_DATA2_MASK                   EQU 0x00ff0000
SI_TX_DATA0_DATA1_MSB                    EQU 15
SI_TX_DATA0_DATA1_LSB                    EQU 8
SI_TX_DATA0_DATA1_MASK                   EQU 0x0000ff00
SI_TX_DATA0_DATA0_MSB                    EQU 7
SI_TX_DATA0_DATA0_LSB                    EQU 0
SI_TX_DATA0_DATA0_MASK                   EQU 0x000000ff

SI_TX_DATA1_ADDRESS                      EQU 0x0c00c00c
SI_TX_DATA1_DATA7_MSB                    EQU 31
SI_TX_DATA1_DATA7_LSB                    EQU 24
SI_TX_DATA1_DATA7_MASK                   EQU 0xff000000
SI_TX_DATA1_DATA6_MSB                    EQU 23
SI_TX_DATA1_DATA6_LSB                    EQU 16
SI_TX_DATA1_DATA6_MASK                   EQU 0x00ff0000
SI_TX_DATA1_DATA5_MSB                    EQU 15
SI_TX_DATA1_DATA5_LSB                    EQU 8
SI_TX_DATA1_DATA5_MASK                   EQU 0x0000ff00
SI_TX_DATA1_DATA4_MSB                    EQU 7
SI_TX_DATA1_DATA4_LSB                    EQU 0
SI_TX_DATA1_DATA4_MASK                   EQU 0x000000ff

SI_RX_DATA0_ADDRESS                      EQU 0x0c00c010
SI_RX_DATA0_DATA3_MSB                    EQU 31
SI_RX_DATA0_DATA3_LSB                    EQU 24
SI_RX_DATA0_DATA3_MASK                   EQU 0xff000000
SI_RX_DATA0_DATA2_MSB                    EQU 23
SI_RX_DATA0_DATA2_LSB                    EQU 16
SI_RX_DATA0_DATA2_MASK                   EQU 0x00ff0000
SI_RX_DATA0_DATA1_MSB                    EQU 15
SI_RX_DATA0_DATA1_LSB                    EQU 8
SI_RX_DATA0_DATA1_MASK                   EQU 0x0000ff00
SI_RX_DATA0_DATA0_MSB                    EQU 7
SI_RX_DATA0_DATA0_LSB                    EQU 0
SI_RX_DATA0_DATA0_MASK                   EQU 0x000000ff

SI_RX_DATA1_ADDRESS                      EQU 0x0c00c014
SI_RX_DATA1_DATA7_MSB                    EQU 31
SI_RX_DATA1_DATA7_LSB                    EQU 24
SI_RX_DATA1_DATA7_MASK                   EQU 0xff000000
SI_RX_DATA1_DATA6_MSB                    EQU 23
SI_RX_DATA1_DATA6_LSB                    EQU 16
SI_RX_DATA1_DATA6_MASK                   EQU 0x00ff0000
SI_RX_DATA1_DATA5_MSB                    EQU 15
SI_RX_DATA1_DATA5_LSB                    EQU 8
SI_RX_DATA1_DATA5_MASK                   EQU 0x0000ff00
SI_RX_DATA1_DATA4_MSB                    EQU 7
SI_RX_DATA1_DATA4_LSB                    EQU 0
SI_RX_DATA1_DATA4_MASK                   EQU 0x000000ff

ENDIF

END
