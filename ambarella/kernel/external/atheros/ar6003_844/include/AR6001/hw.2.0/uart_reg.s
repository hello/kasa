IF :LNOT: :DEF: _UART_REG_S
_UART_REG_S EQU 1

RBR_ADDRESS                              EQU 0x0c008000
RBR_RBR_MSB                              EQU 7
RBR_RBR_LSB                              EQU 0
RBR_RBR_MASK                             EQU 0x000000ff

THR_ADDRESS                              EQU 0x0c008000
THR_THR_MSB                              EQU 7
THR_THR_LSB                              EQU 0
THR_THR_MASK                             EQU 0x000000ff

DLL_ADDRESS                              EQU 0x0c008000
DLL_DLL_MSB                              EQU 7
DLL_DLL_LSB                              EQU 0
DLL_DLL_MASK                             EQU 0x000000ff

DLH_ADDRESS                              EQU 0x0c008004
DLH_DLH_MSB                              EQU 7
DLH_DLH_LSB                              EQU 0
DLH_DLH_MASK                             EQU 0x000000ff

IER_ADDRESS                              EQU 0x0c008004
IER_EDDSI_MSB                            EQU 3
IER_EDDSI_LSB                            EQU 3
IER_EDDSI_MASK                           EQU 0x00000008
IER_ELSI_MSB                             EQU 2
IER_ELSI_LSB                             EQU 2
IER_ELSI_MASK                            EQU 0x00000004
IER_ETBEI_MSB                            EQU 1
IER_ETBEI_LSB                            EQU 1
IER_ETBEI_MASK                           EQU 0x00000002
IER_ERBFI_MSB                            EQU 0
IER_ERBFI_LSB                            EQU 0
IER_ERBFI_MASK                           EQU 0x00000001

IIR_ADDRESS                              EQU 0x0c008008
IIR_FIFO_STATUS_MSB                      EQU 7
IIR_FIFO_STATUS_LSB                      EQU 6
IIR_FIFO_STATUS_MASK                     EQU 0x000000c0
IIR_IID_MSB                              EQU 3
IIR_IID_LSB                              EQU 0
IIR_IID_MASK                             EQU 0x0000000f

FCR_ADDRESS                              EQU 0x0c008008
FCR_RCVR_TRIG_MSB                        EQU 7
FCR_RCVR_TRIG_LSB                        EQU 6
FCR_RCVR_TRIG_MASK                       EQU 0x000000c0
FCR_DMA_MODE_MSB                         EQU 3
FCR_DMA_MODE_LSB                         EQU 3
FCR_DMA_MODE_MASK                        EQU 0x00000008
FCR_XMIT_FIFO_RST_MSB                    EQU 2
FCR_XMIT_FIFO_RST_LSB                    EQU 2
FCR_XMIT_FIFO_RST_MASK                   EQU 0x00000004
FCR_RCVR_FIFO_RST_MSB                    EQU 1
FCR_RCVR_FIFO_RST_LSB                    EQU 1
FCR_RCVR_FIFO_RST_MASK                   EQU 0x00000002
FCR_FIFO_EN_MSB                          EQU 0
FCR_FIFO_EN_LSB                          EQU 0
FCR_FIFO_EN_MASK                         EQU 0x00000001

LCR_ADDRESS                              EQU 0x0c00800c
LCR_DLAB_MSB                             EQU 7
LCR_DLAB_LSB                             EQU 7
LCR_DLAB_MASK                            EQU 0x00000080
LCR_BREAK_MSB                            EQU 6
LCR_BREAK_LSB                            EQU 6
LCR_BREAK_MASK                           EQU 0x00000040
LCR_EPS_MSB                              EQU 4
LCR_EPS_LSB                              EQU 4
LCR_EPS_MASK                             EQU 0x00000010
LCR_PEN_MSB                              EQU 3
LCR_PEN_LSB                              EQU 3
LCR_PEN_MASK                             EQU 0x00000008
LCR_STOP_MSB                             EQU 2
LCR_STOP_LSB                             EQU 2
LCR_STOP_MASK                            EQU 0x00000004
LCR_CLS_MSB                              EQU 1
LCR_CLS_LSB                              EQU 0
LCR_CLS_MASK                             EQU 0x00000003

MCR_ADDRESS                              EQU 0x0c008010
MCR_LOOPBACK_MSB                         EQU 5
MCR_LOOPBACK_LSB                         EQU 5
MCR_LOOPBACK_MASK                        EQU 0x00000020
MCR_OUT2_MSB                             EQU 3
MCR_OUT2_LSB                             EQU 3
MCR_OUT2_MASK                            EQU 0x00000008
MCR_OUT1_MSB                             EQU 2
MCR_OUT1_LSB                             EQU 2
MCR_OUT1_MASK                            EQU 0x00000004
MCR_RTS_MSB                              EQU 1
MCR_RTS_LSB                              EQU 1
MCR_RTS_MASK                             EQU 0x00000002
MCR_DTR_MSB                              EQU 0
MCR_DTR_LSB                              EQU 0
MCR_DTR_MASK                             EQU 0x00000001

LSR_ADDRESS                              EQU 0x0c008014
LSR_FERR_MSB                             EQU 7
LSR_FERR_LSB                             EQU 7
LSR_FERR_MASK                            EQU 0x00000080
LSR_TEMT_MSB                             EQU 6
LSR_TEMT_LSB                             EQU 6
LSR_TEMT_MASK                            EQU 0x00000040
LSR_THRE_MSB                             EQU 5
LSR_THRE_LSB                             EQU 5
LSR_THRE_MASK                            EQU 0x00000020
LSR_BI_MSB                               EQU 4
LSR_BI_LSB                               EQU 4
LSR_BI_MASK                              EQU 0x00000010
LSR_FE_MSB                               EQU 3
LSR_FE_LSB                               EQU 3
LSR_FE_MASK                              EQU 0x00000008
LSR_PE_MSB                               EQU 2
LSR_PE_LSB                               EQU 2
LSR_PE_MASK                              EQU 0x00000004
LSR_OE_MSB                               EQU 1
LSR_OE_LSB                               EQU 1
LSR_OE_MASK                              EQU 0x00000002
LSR_DR_MSB                               EQU 0
LSR_DR_LSB                               EQU 0
LSR_DR_MASK                              EQU 0x00000001

MSR_ADDRESS                              EQU 0x0c008018
MSR_DCD_MSB                              EQU 7
MSR_DCD_LSB                              EQU 7
MSR_DCD_MASK                             EQU 0x00000080
MSR_RI_MSB                               EQU 6
MSR_RI_LSB                               EQU 6
MSR_RI_MASK                              EQU 0x00000040
MSR_DSR_MSB                              EQU 5
MSR_DSR_LSB                              EQU 5
MSR_DSR_MASK                             EQU 0x00000020
MSR_CTS_MSB                              EQU 4
MSR_CTS_LSB                              EQU 4
MSR_CTS_MASK                             EQU 0x00000010
MSR_DDCD_MSB                             EQU 3
MSR_DDCD_LSB                             EQU 3
MSR_DDCD_MASK                            EQU 0x00000008
MSR_TERI_MSB                             EQU 2
MSR_TERI_LSB                             EQU 2
MSR_TERI_MASK                            EQU 0x00000004
MSR_DDSR_MSB                             EQU 1
MSR_DDSR_LSB                             EQU 1
MSR_DDSR_MASK                            EQU 0x00000002
MSR_DCTS_MSB                             EQU 0
MSR_DCTS_LSB                             EQU 0
MSR_DCTS_MASK                            EQU 0x00000001

ENDIF

END
