

#include<STC15.h>
#include<intrins.h> 
#define uint  unsigned int
#define uchar unsigned char
#define uchar unsigned char
#define unt unsigned int



sbit cs=P2^2; 
sbit clk=P2^3; 
sbit din=P2^4; 
sbit reset =P2^1;//注意 若模组无RESET 则模组内已经内置RC复位电路。无需此IO






unsigned char  ziku_data[][5]  ={	
 0x7f,0x7f,0x41,0x7f,0x7f, // 0
 0x00,0x06,0x7f,0x7f,0x00, // 1
 0x79,0x79,0x49,0x4f,0x4f, //	2
 0x49,0x49,0x49,0x7f,0x7f, //	3
 0x0f,0x0f,0x08,0x7f,0x7f, //	4
 0x4f,0x4f,0x49,0x79,0x79, //	5
 0x7f,0x7f,0x49,0x79,0x78, // 6
 0x01,0x01,0x71,0x7f,0x0f, // 7
 0x7f,0x7f,0x49,0x7f,0x7f, //	8
 0x0f,0x4f,0x49,0x7f,0x7f, //	9
 0x00,0x00,0x36,0x36,0x00, // : 10

  };



                                                                                              
uchar liang;	   //调整


int  temp8;

	
/////////////////////////////////////////////////////////////////////////
void delay();  

void write_6302(unsigned char w_data); 

void VFD_cmd(unsigned char command); 

void VFD_addr(unsigned char addr); 

void S1201_show(void);

void S1201_WriteOneChar(unsigned char x, unsigned char chr);

void S1201_WriteStr(unsigned char x, char *str);

void S1201_WriteUserFont(unsigned char x,unsigned char y, unsigned char *str);

void delay2(); 

void delay3(); 

void VFD_init();

/////////////////////////////////////////////////////////////////////////

void delayx(uint z)	   //延时子程序 约1MS
{
   uint x,y;
   for(x=z;x>0;x--)
   for(y=1000;y>0;y--);
}

void delay() //2US

{  
unsigned char i;

	i = 3;
	while (--i);

} 

void delay3(uint z)
{ 
 
   uint x,y;
   for(x=z;x>0;x--)
   for(y=1000;y>0;y--);
}

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
     clk=1;    
   
	}
 }
 
 
  
void VFD_cmd(unsigned char command) 
{ 
  cs=0;    
  write_6302(command);    
	cs=1;    
	delay();
} 

void VFD_init()
 { 
   cs=0;
	 write_6302(0xe0);  
	 write_6302(0x0f);//显示位数 16位0X0F
	 cs=1;
	 delay();

	 cs=0;
	 write_6302(0xe4); 
	 write_6302(0xff);//亮度0-255
	 cs=1;
	 delay();
//	VFD_cmd(0xe9);
	} 


/******************************
*用于更新当前屏幕显示
*******************************/
void S1201_show(void)
{
  cs=0; //开始传输
  write_6302(0xe8);//地址寄存器起始位置
  cs=1; //停止传输
}

/******************************
*用于亮度设定
*******************************/
void liangset()
{

  cs=0;
  write_6302(0xe4);  
  write_6302(liang);//亮度值数据
  cs=1;
  delay();
		
}

/******************************
*在指定位置打印一个字符(用户自定义,所有CG-ROM中的)
*x:0~11;chr:要显示的字符编码
*******************************/
void S1201_WriteOneChar(unsigned char x, unsigned char chr)
{
  cs=0; //开始传输 
  write_6302(0x20+x);//地址寄存器起始位置
  write_6302(chr+0x30); 
  cs=1; //停止传输
	//S1201_show();	
}



/******************************
*在指定位置打印字符串
*(仅适用于英文,标点,数字)
*x:0~11;str:要显示的字符串
*******************************/
void S1201_WriteStr(unsigned char x, char *str)
{
  cs=0; //开始传输 
  write_6302(0x20+x);//地址寄存器起始位置
	
  while (*str) 
  {
    write_6302(*str); //ascii与对应字符表转换 
    str++;
  } 
  cs=1; //停止传输
	
	//S1201_show();	
}

/******************************
*自定义字符(总共支持8个自定义字符)
*取模时,自上到下:低位到高位;自左到右;
*x:0~7;str:字模(5个8位的数组)
*******************************/
void S1201_WriteUserFont(unsigned char x, unsigned char y,unsigned char *s)
{
	unsigned char i=0;
		unsigned char ii=0;
  cs=0; //开始传输 
  write_6302(0x40+y);//地址寄存器起始位置
	
    for(i=0;i<5;i++)
     write_6302(s[i]);
  cs=1; //停止传输

  cs=0;
  write_6302(0x20+x);
  write_6302(0x00+y);   
  cs=1;

	//S1201_show();
}


void main() 
{      
   
  P1M0=0x00; 
	P1M1=0x00;    
	P2M0=0x00; 
	P2M1=0x00;
	P3M0=0x00;
  P3M1=0x00;


  reset=0;
	delay3(50);//注意reset时间是否满足屏reset要求时序
  reset=1;
	
	VFD_init();
  VFD_cmd(0xE9);// all test
  delayx(1000);
	
  VFD_init();
   
   while(1)
 {
   S1201_WriteOneChar(0,0);//首位字符0
	 S1201_WriteStr(1,"L");//2号位字符L
	 S1201_WriteUserFont(2,2,ziku_data[2]);//3号位字符 自定义字库中字模
	 S1201_WriteUserFont(3,3,ziku_data[3]);
	 S1201_WriteUserFont(4,4,ziku_data[4]);
	 S1201_WriteUserFont(5,5,ziku_data[5]);
	 S1201_WriteUserFont(6,6,ziku_data[6]);
	 S1201_WriteUserFont(7,7,ziku_data[7]);
   S1201_show();
	 
  }	  //while
}	 //main


