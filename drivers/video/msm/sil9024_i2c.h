#ifndef _SIL9024_I2C_H_
#define SIL9024A_H

extern int32 sil9024a_i2c_read(unsigned short raddr, unsigned char *rdata);
extern int32 sil9024a_i2c_read_len(byte deviceID, word raddr, byte *rdata, word length);
extern int32 sil9024a_i2c_write(byte deviceID, byte raddr, byte wdata);
extern int32 sil9024a_i2c_writeAddr(byte deviceID, byte raddr, byte dummy_data);
extern int32 sil9024a_i2c_read_segment(byte deviceID, byte segment, byte raddr, byte *rdata, word length);


extern int32 sil9024a_ddc_read(unsigned short raddr, unsigned char *rdata);
extern int32 sil9024a_ddc_read_len(word raddr, byte *rdata, word length);
extern int32 sil9024a_ddc_write(unsigned short raddr, unsigned short wdata);

extern void sky_hdmi_i2c_drv_register(void);

#endif /* _SIL9024_I2C_H_ */
