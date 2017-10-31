
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//#define CPSR_N (1<<31)
#define CPSR_Z (1<<30)
//#define CPSR_C (1<<29)
//#define CPSR_V (1<<28)
//#define CPSR_Q (1<<27)


#define ROMMASK 0xFFF
#define ROMSIZE (ROMMASK+1)
unsigned short rom[ROMSIZE>>1];

#define RAMMASK 0xFFF
#define RAMSIZE (RAMMASK+1)
unsigned short ram[RAMSIZE>>1];

unsigned int reg_norm[16];
unsigned int cpsr;

void do_zflag ( unsigned int x )
{
    if(x==0) cpsr|=CPSR_Z; else cpsr&=~CPSR_Z;
}


unsigned int read16 ( unsigned int addr )
{
    unsigned int data;

    switch(addr&0xF0000000)
    {
        case 0x00000000: //ROM
            addr&=ROMMASK;
            addr>>=1;
            data=rom[addr];
            return(data);
        case 0x20000000: //RAM
            addr&=RAMMASK;
            addr>>=1;
            data=ram[addr];
            return(data);
    }
    fprintf(stderr,"read16(0x%08X), abort\n",addr);
    exit(1);
}

unsigned int read32 ( unsigned int addr )
{
    unsigned int data;

    switch(addr&0xF0000000)
    {
        case 0x00000000: //ROM
        case 0x20000000: //RAM
            data =read16(addr+0);
            data|=((unsigned int)read16(addr+2))<<16;
            return(data);
    }
    fprintf(stderr,"read32(0x%08X), abort\n",addr);
    exit(1);
}

void write16 ( unsigned int addr, unsigned int data )
{
    switch(addr&0xF0000000)
    {
        case 0x20000000: //RAM
            addr&=RAMMASK;
            addr>>=1;
            ram[addr]=data&0xFFFF;
            return;
    }
    fprintf(stderr,"write16(0x%08X,0x%04X), abort\n",addr,data);
    exit(1);
}

void write32 ( unsigned int addr, unsigned int data )
{
    switch(addr&0xF0000000)
    {
        case 0xD0000000:
            printf("0x%08X\n",data);
            return;
        case 0x20000000: //RAM
            write16(addr+0,(data>> 0)&0xFFFF);
            write16(addr+2,(data>>16)&0xFFFF);
            return;
    }
    fprintf(stderr,"write32(0x%08X,0x%08X), abort\n",addr,data);
    exit(1);
}

unsigned int fetch16 ( unsigned int addr )
{
    unsigned int data;

    switch(addr&0xF0000000)
    {
        case 0x00000000: //ROM
            addr&=ROMMASK;
            addr>>=1;
            data=rom[addr];
            return(data);
        case 0x20000000: //RAM
            addr&=RAMMASK;
            addr>>=1;
            data=ram[addr];
            return(data);
    }
    exit(1);
}

unsigned int fetch32 ( unsigned int addr )
{
    unsigned int data;

    switch(addr&0xF0000000)
    {
        case 0x00000000: //ROM
            if(addr<0x50)
            {
                data=read32(addr);
                if(addr==0x00000000) return(data);
                if(addr==0x00000004) return(data);
                if(addr==0x0000003C) return(data);
                fprintf(stderr,"fetch32(0x%08X), unaligned abort\n",addr);
                exit(1);
            }
        case 0x20000000: //RAM
            data=read32(addr);
            return(data);
    }
    exit(1);
}

unsigned int read_register ( unsigned int reg )
{
    unsigned int data;

    reg&=0xF;
    data=reg_norm[reg];
    if(reg==15)
    {
        if(data&1)
        {
            fprintf(stderr,"pc has lsbit set 0x%08X\n",data);
        }
        data&=~1;
    }
    return(data);
}

void write_register ( unsigned int reg, unsigned int data )
{
    reg&=0xF;
    if(reg==15) data&=~1;
    reg_norm[reg]=data;
}


unsigned int execute ( void )
{
    unsigned int pc;
    unsigned int sp;
    unsigned int inst;

    unsigned int ra,rb,rc;
    unsigned int rm,rd,rn,rs;
    unsigned int op;

    pc=read_register(15);
    inst=fetch16(pc-2);
    pc+=2;
    write_register(15,pc);

    fprintf(stderr,"--- 0x%08X: 0x%04X ",(pc-4),inst);


    //ADD(1) small immediate two registers
    if((inst&0xFE00)==0x1C00)
    {
        rd=(inst>>0)&0x7;
        rn=(inst>>3)&0x7;
        rb=(inst>>6)&0x7;
        if(rb)
        {
            fprintf(stderr,"adds r%u,r%u,#0x%X\n",rd,rn,rb);
            ra=read_register(rn);
            rc=ra+rb;
            write_register(rd,rc);
            do_zflag(rc);
            return(0);
        }
        else
        {
            //this is a mov
        }
    }

    //ADD(2) big immediate one register
    if((inst&0xF800)==0x3000)
    {
        rb=(inst>>0)&0xFF;
        rd=(inst>>8)&0x7;
        fprintf(stderr,"adds r%u,#0x%02X\n",rd,rb);
        ra=read_register(rd);
        rc=ra+rb;
        write_register(rd,rc);
        do_zflag(rc);
        return(0);
    }


    //B(1) conditional branch
    if((inst&0xF000)==0xD000)
    {
        rb=(inst>>0)&0xFF;
        if(rb&0x80) rb|=(~0)<<8;
        op=(inst>>8)&0xF;
        rb<<=1;
        rb+=pc;
        rb+=2;
        switch(op)
        {
            case 0x1: //b ne  z clear
                fprintf(stderr,"bne 0x%08X\n",rb-3);
                if(!(cpsr&CPSR_Z))
                {
                    write_register(15,rb);
                }
                return(0);
        }
    }

    //B(2) unconditional branch
    if((inst&0xF800)==0xE000)
    {
        rb=(inst>>0)&0x7FF;
        if(rb&(1<<10)) rb|=(~0)<<11;
        rb<<=1;
        rb+=pc;
        rb+=2;
        fprintf(stderr,"b 0x%08X\n",rb-2);
        write_register(15,rb);
        return(0);
    }

    //BL
    if((inst&0xF000)==0xF000) //BL
    {
        if((inst&0x0800)==0x0000) //H=b10
        {
            fprintf(stderr,"\n");
            rb=inst&((1<<11)-1);
            if(rb&1<<10) rb|=(~((1<<11)-1)); //sign extend
            rb<<=12;
            rb+=pc;
            write_register(14,rb);
            return(0);
        }
        else
        if((inst&0x0800)==0x0800) //H=b11
        {
            rb=read_register(14);
            rb+=(inst&((1<<11)-1))<<1;;
            rb+=2;

            fprintf(stderr,"bl 0x%08X\n",rb-2);
            write_register(14,(pc-2)|1);
            write_register(15,rb);
            return(0);
        }
    }

    //BKPT
    if((inst&0xFF00)==0xBE00)
    {
        rb=(inst>>0)&0xFF;
        fprintf(stderr,"bkpt 0x%02X\n",rb);
        return(1);
    }

    //BX
    if((inst&0xFF87)==0x4700)
    {
        rm=(inst>>3)&0xF;
        fprintf(stderr,"bx r%u\n",rm);
        rc=read_register(rm);
        rc+=2;
        if(rc&1)
        {
            rc&=~1;
            write_register(15,rc);
            return(0);
        }
        else
        {
            fprintf(stderr,"cannot branch to arm 0x%08X 0x%04X\n",pc,inst);
            return(1);
        }
    }

    //CMP(1) compare immediate
    if((inst&0xF800)==0x2800)
    {
        rb=(inst>>0)&0xFF;
        rn=(inst>>8)&0x07;
        fprintf(stderr,"cmp r%u,#0x%02X\n",rn,rb);
        ra=read_register(rn);
        rc=ra-rb;
        do_zflag(rc);
        return(0);
    }

    //LDR(3)
    if((inst&0xF800)==0x4800)
    {
        rb=(inst>>0)&0xFF;
        rd=(inst>>8)&0x07;
        rb<<=2;
        fprintf(stderr,"ldr r%u,[PC+#0x%X] ",rd,rb);
        ra=read_register(15);
        ra&=~3;
        rb+=ra;
        fprintf(stderr,";@ 0x%X\n",rb);
        rc=read32(rb);
        write_register(rd,rc);
        return(0);
    }

    //LSL(1)
    if((inst&0xF800)==0x0000)
    {
        rd=(inst>>0)&0x07;
        rm=(inst>>3)&0x07;
        rb=(inst>>6)&0x1F;
        fprintf(stderr,"lsls r%u,r%u,#0x%X\n",rd,rm,rb);
        rc=read_register(rm);
        if(rb==0)
        {
            //if immed_5 == 0
            //C unnaffected
            //result not shifted
        }
        else
        {
            //else immed_5 > 0
            //C affected
            rc<<=rb;
        }
        write_register(rd,rc);
        do_zflag(rc);
        return(0);
    }


    //POP
    if((inst&0xFE00)==0xBC00)
    {
        fprintf(stderr,"pop {");
        for(ra=0,rb=0x01,rc=0;rb;rb=(rb<<1)&0xFF,ra++)
        {
            if(inst&rb)
            {
                if(rc) fprintf(stderr,",");
                fprintf(stderr,"r%u",ra);
                rc++;
            }
        }
        if(inst&0x100)
        {
            if(rc) fprintf(stderr,",");
            fprintf(stderr,"pc");
        }
        fprintf(stderr,"}\n");

        sp=read_register(13);
        for(ra=0,rb=0x01;rb;rb=(rb<<1)&0xFF,ra++)
        {
            if(inst&rb)
            {
                write_register(ra,read32(sp));
                sp+=4;
            }
        }
        if(inst&0x100)
        {
            rc=read32(sp);
            if((rc&1)==0)
            {
                fprintf(stderr,"pop {rc} with an ARM address pc 0x%08X popped 0x%08X\n",pc,rc);
                //exit(1);
                rc&=~1;
            }
            rc+=2;
            write_register(15,rc);
            sp+=4;
        }
        write_register(13,sp);
        return(0);
    }



    //PUSH
    if((inst&0xFE00)==0xB400)
    {
        fprintf(stderr,"push {");
        for(ra=0,rb=0x01,rc=0;rb;rb=(rb<<1)&0xFF,ra++)
        {
            if(inst&rb)
            {
                if(rc) fprintf(stderr,",");
                fprintf(stderr,"r%u",ra);
                rc++;
            }
        }
        if(inst&0x100)
        {
            if(rc) fprintf(stderr,",");
            fprintf(stderr,"lr");
        }
        fprintf(stderr,"}\n");

        sp=read_register(13);
        for(ra=0,rb=0x01,rc=0;rb;rb=(rb<<1)&0xFF,ra++)
        {
            if(inst&rb)
            {
                rc++;
            }
        }
        if(inst&0x100) rc++;
        rc<<=2;
        sp-=rc;
        rd=sp;
        for(ra=0,rb=0x01;rb;rb=(rb<<1)&0xFF,ra++)
        {
            if(inst&rb)
            {
                write32(rd,read_register(ra));
                rd+=4;
            }
        }
        if(inst&0x100)
        {
            rc=read_register(14);
            write32(rd,rc); //read_register(14));

            if((rc&1)==0)
            {
                fprintf(stderr,"push {lr} with an ARM address pc 0x%08X popped 0x%08X\n",pc,rc);
            }
        }
        write_register(13,sp);
        return(0);
    }


    //STR(1)
    if((inst&0xF800)==0x6000)
    {
        rd=(inst>>0)&0x07;
        rn=(inst>>3)&0x07;
        rb=(inst>>6)&0x1F;
        rb<<=2;
        fprintf(stderr,"str r%u,[r%u,#0x%X]\n",rd,rn,rb);
        rb=read_register(rn)+rb;
        rc=read_register(rd);
        write32(rb,rc);
        return(0);
    }



    //MOV(1) immediate
    if((inst&0xF800)==0x2000)
    {
        rb=(inst>>0)&0xFF;
        rd=(inst>>8)&0x07;
        fprintf(stderr,"movs r%u,#0x%02X\n",rd,rb);
        write_register(rd,rb);
        do_zflag(rb);
        return(0);
    }

    //MOV(2) two low registers
    if((inst&0xFFC0)==0x1C00)
    {
        rd=(inst>>0)&7;
        rn=(inst>>3)&7;
        fprintf(stderr,"movs r%u,r%u\n",rd,rn);
        rc=read_register(rn);
        write_register(rd,rc);
        do_zflag(rc);
        return(0);
    }




    fprintf(stderr,"unknown instruction\n");
    return(1);
}

int main ( int argc, char *argv[] )
{
    unsigned int ra;
    unsigned int rb;
    FILE *fp;

    if(argc<2)
    {
        fprintf(stderr,".bin file not specified\n");
        return(1);
    }
    memset(rom,0xFF,sizeof(rom));
    memset(ram,0xCA,sizeof(ram));
    fp=fopen(argv[1],"rb");
    if(fp==NULL)
    {
        fprintf(stderr,"Error opening file [%s]\n",argv[1]);
        return(1);
    }
    rb=fread(rom,1,ROMSIZE,fp);
    printf("%u bytes read\n",rb);
    fclose(fp);
    //reset
    memset(reg_norm,0xDA,sizeof(reg_norm));
    cpsr=0;
    reg_norm[13]=fetch32(0x00000000);
    reg_norm[14]=0xFFFFFFFF;
    reg_norm[15]=fetch32(0x00000004);
    if((reg_norm[15]&1)==0)
    {
        fprintf(stderr,"reset vector with an ARM address 0x%08X\n",reg_norm[15]);
        exit(1);
    }
    reg_norm[15]&=~1;
    reg_norm[15]+=2;

    while(1)
    {
        if(execute()) break;
    }


    return(0);
}


