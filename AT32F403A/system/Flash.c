#include "at32f403a_407.h"
#include "Flash.h"
//  Attention: flash擦写次数为10万次，故不可在死循环中反复调用flash函数  //

/**
  * @brief   flash写入数据
  * @param   add 32位flash地址
  * @param	 dat 16位数据
  * @retval  无
  */
void FLASH_W(uint32_t add, uint8_t dat1, uint8_t dat2, uint8_t dat3, uint8_t dat4)
{
    flash_unlock();
    flash_flag_clear(FLASH_OBF_FLAG | FLASH_ODF_FLAG | FLASH_PRGMERR_FLAG | FLASH_EPPERR_FLAG);
    flash_sector_erase(add);
    flash_halfword_program(add, dat1);
    flash_halfword_program(add + 2, dat2);
    flash_halfword_program(add + 4, dat3);
    flash_halfword_program(add + 6, dat4);
    flash_flag_clear(FLASH_OBF_FLAG | FLASH_ODF_FLAG | FLASH_PRGMERR_FLAG | FLASH_EPPERR_FLAG);
    flash_lock();
}

/**
  * @brief   flash写入数据
  * @param   add 32位flash地址
  * @param	 dat 16位数据，合计64位卡号，number数量
  * @retval  无
  */
void FLASH_W3(uint32_t add, uint8_t dat1, uint8_t dat2, uint8_t dat3, uint8_t dat4, uint8_t number)
{
    flash_unlock(); //解锁FLASH编程擦除控制器
    flash_flag_clear(FLASH_OBF_FLAG | FLASH_ODF_FLAG | FLASH_PRGMERR_FLAG | FLASH_EPPERR_FLAG); //清除标志位
    flash_sector_erase(add);    //擦除指定地址页
    flash_halfword_program(add, dat1); //从指定页的addr地址开始写
    flash_halfword_program(add + 2, dat2);
    flash_halfword_program(add + 4, dat3);
    flash_halfword_program(add + 6, dat4);
    flash_halfword_program(add + 8, number);
    flash_flag_clear(FLASH_OBF_FLAG | FLASH_ODF_FLAG | FLASH_PRGMERR_FLAG | FLASH_EPPERR_FLAG); //清除标志位
    flash_lock();    //锁定FLASH编程擦除控制器
}

/**
  * @brief   flash写入数据
  * @param   add 32位flash地址
  * @param	 dat 16位数组，合计12个字节卡号
  * @retval  无
  */
void FLASH_W4(uint32_t add, u8* dat)
{
    flash_unlock(); //解锁FLASH编程擦除控制器
    flash_flag_clear(FLASH_OBF_FLAG | FLASH_ODF_FLAG | FLASH_PRGMERR_FLAG | FLASH_EPPERR_FLAG); //清除标志位
    flash_sector_erase(add);    //擦除指定地址页
    flash_halfword_program(add, dat[0]); //从指定页的addr地址开始写
    flash_halfword_program(add + 2, dat[1]);
    flash_halfword_program(add + 4, dat[2]);
    flash_halfword_program(add + 6, dat[3]);
    flash_halfword_program(add + 8, dat[4]);
    flash_halfword_program(add + 10, dat[5]);
    flash_halfword_program(add + 12, dat[6]);
    flash_halfword_program(add + 14, dat[7]);
    flash_halfword_program(add + 16, dat[8]);
    flash_halfword_program(add + 18, dat[9]);
    flash_halfword_program(add + 20, dat[10]);
    flash_halfword_program(add + 22, dat[11]);
    flash_flag_clear(FLASH_OBF_FLAG | FLASH_ODF_FLAG | FLASH_PRGMERR_FLAG | FLASH_EPPERR_FLAG); //清除标志位
    flash_lock();    //锁定FLASH编程擦除控制器
}

/**
  * @brief   flash写入数据
  * @param   add 32位flash地址
  * @param	 dat 16位数据
  * @retval  无
  */
void FLASH_W2(uint32_t add, uint8_t dat)
{
    flash_unlock(); //解锁FLASH编程擦除控制器
    flash_flag_clear(FLASH_OBF_FLAG | FLASH_ODF_FLAG | FLASH_PRGMERR_FLAG | FLASH_EPPERR_FLAG); //清除标志位
    flash_sector_erase(add);    //擦除指定地址页
    flash_halfword_program(add, dat); //从指定页的addr地址开始写
    flash_flag_clear(FLASH_OBF_FLAG | FLASH_ODF_FLAG | FLASH_PRGMERR_FLAG | FLASH_EPPERR_FLAG); //清除标志位
    flash_lock();    //锁定FLASH编程擦除控制器
}

/**
  * @brief    FLASH读出数据
  * @param    add 32位读出FLASH地址
  * @retval   16位数据
  */
uint16_t FLASH_R(uint32_t add)
{
    uint16_t a;
    a = *(uint16_t*)add;
    return a;
}

/**
  * @brief    FLASH读出数据
  * @param    add 32位读出FLASH地址
  * @retval   8位数据
  */
uint8_t FLASH_R2(uint32_t add)
{
    uint8_t a;
    a = *(uint8_t*)add;
    return a;
}

/**
  * @brief    擦除指定FLASH地址页内的内容
  * @param    add 32位FLASH地址
  * @retval   无
  */
void FLASH_Clear(uint32_t add)
{
    flash_unlock(); //解锁FLASH编程擦除控制器
    flash_flag_clear(FLASH_OBF_FLAG | FLASH_ODF_FLAG | FLASH_PRGMERR_FLAG | FLASH_EPPERR_FLAG); //清除标志位
    flash_sector_erase(add);    //擦除指定地址页
    flash_flag_clear(FLASH_OBF_FLAG | FLASH_ODF_FLAG | FLASH_PRGMERR_FLAG | FLASH_EPPERR_FLAG); //清除标志位
    flash_lock();
}

