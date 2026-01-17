
#include "reg52.h"			 //此文件中定义了单片机的一些特殊功能寄存器
	

typedef unsigned int u16;	  //对数据类型进行声明定义
typedef unsigned char u8 ;

sbit reset =P1^1;		//复位
sbit cs   = P1^2; 		//片选
sbit clk  = P1^3; 		//串行时钟
sbit din  = P1^4; 		//串行数据
sbit EN   = P1^X; 			//电源使能，请初始化前上拉到高电平，或 直接模块焊盘J1短接

void delay() //2US

{  
    unsigned char i;
i = 3;
while (--i);

}

void delay1(uint z)	   //延时子程序 约1MS
{
   uint x,y;
   for(x=z;x>0;x--)
   for(y=1000;y>0;y--);
}
/******************************
*用于8位数据/命令传输
*******************************/
void write_6302(unsigned char w_data)
 { 
    unsigned char i;      
    for(i=0;i<8;i++)   
  {  
        clk=0;    
        if( (w_data&0x01) == 0x01)       
  { 
           din=1;      
      }        
else       
 { 
            din=0;    
  } 
     w_data>>=1;      
     delay();      
     clk=1;    
     delay(); 
}
 }
/******************************
*初始化
*******************************/
void VFD_init()
 { 
  cs=0;
  write_6302(0xe0); 
  delay();  
  write_6302(0x07);//DIM 0~7 
  cs=1;
  delay();

  cs=0;
  write_6302(0xe4); 
  delay();  
  write_6302(0xff);//bright	    write_6302(0xff)
  cs=1;
  delay();
  }

/******************************
*用于更新当前屏幕显示
*******************************/
void S1201_show(void)
{
  cs=0; //开始传输
  delay();    //延时300ns 
  write_6302(0xe8);//地址寄存器起始位置
  cs=1; //停止传输
}
/******************************
*在指定位置打印一个字符(用户自定义,所有CG-ROM中的)
*x:0~11;chr:要显示的字符编码
*******************************/
void S1201_WriteOneChar(unsigned char x, unsigned char chr)
{
  cs=0; //开始传输
  delay();     //延时300ns  
  write_6302(0x20+x);//地址寄存器起始位置
  delay(); 
  write_6302(chr+0x30); 
  cs=1; //停止传输	
 // S1201_show();	
}
/******************************
*在指定位置打印字符串
*(仅适用于英文,标点,数字)
*x:0~11;str:要显示的字符串
*******************************/
void S1201_WriteStr(unsigned char x, char *str)
{
  cs=0; //开始传输
  delay();    //延时300ns
  write_6302(0x20+x);//地址寄存器起始位置	
  while (*str) 
  {
    write_6302(*str); //ascii与对应字符表转换    
    str++;
  }  
  cs=1; //停止传输	
  //S1201_show();	
}




/*******************************************************************************
* 函 数 名       : main
* 函数功能		 : 主函数
* 输    入       : 无
* 输    出    	 : 无
*******************************************************************************/
void main()
{	
  reset=0;	
	delay1(300);
	reset=1;	
	VFD_init();

	while(1)
	{
	
	//亮度//	
  cs=0;
  write_6302(0xe4); 
  delay();  
  write_6302(0xff);//bright 0-255
  cs=1;
  delay();
	//亮度//

		S1201_WriteOneChar(0,0);
		S1201_WriteOneChar(1,1);
		S1201_WriteOneChar(2,2);
		S1201_WriteOneChar(3,3);
		S1201_WriteOneChar(4,4);
		S1201_WriteOneChar(5,5);
		S1201_WriteOneChar(6,6);
		S1201_WriteOneChar(7,7);
		 S1201_show();	
   
		 delay1(1000);
		 S1201_WriteStr(0,"ABCDEFGH")	;
		 S1201_show();
		 delay1(1000);
	 
 }

}










