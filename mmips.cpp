#include <iostream>
#include <string>
#include <fstream>
#include <map>
#include <cmath>
#include <functional>
#include <string.h>

using namespace std;

map<int, string> mm;
int mapnum = 0;

int main_line = 0;
unsigned int label_line[10000];
unsigned int label_line_num = 0;
unsigned int label_add[10000];

int reg[35] = {0};  // 32 = hi, 33 = lo, 34 = pc;

struct xxx
{
    int type = 0;
    bool isnum = false;
    int num[3] = {0};


    &operator=(const xxx &other)
    {
        type = other.type;
        isnum = other.isnum;
        num[0] = other.num[0];
        num[1] = other.num[1];
        num[2] = other.num[2];
    }

} line[15000];



string re_label[5000];
int re_label_line[5000];
int re_label_num = 0;


unsigned char mem[4194304];  // 1024*1024*4
int mem_pos = 0;
/**
        第一个是存 wb回去的对应寄存器编号（Rdest）， 若无需这一步为-1；
        后面为转换的相应的数字
        rsrc—寄存器里的数
        src-寄存器里的数或立即数
        imm-数
        label-对应的行号
        address-地址
        */
/**
    type:
        1   nop
        2   syscall
        3   .data
        4   .text               //no
        5   .align
        6   .ascii
        7   .asciiz
        8   .byte / sb 1 2 3 ...
        9   .half / sh 1 2 3 ...
        10  .word / sw 1 2 3 ...
        11  .space              //.
        12  b/j                 // num[2] 存地址 num[0] label_line[i] 的那个i
        13  jr
        14  jal
        15  jalr                //jalr/jal 0 - re_label_num 2 地址
        16  mfhi
        17  mflo                //one
        18  mul(two)
        19  mulu(two)
        20  div(two)
        21  divu(two)
        22  neg
        23  negu
        24  li
        25  beqz
        26  bnez
        27  blez
        28  bgez
        29  bgtz
        30  bltz
        31  la                  //num[0]res num[1]offset num[2]res/_label
        32  lb
        33  lh
        34  lw
        35  sb
        36  sh
        37  sw                    //two
        38  add
        39  addu
        40  addiu
        41  sub
        42  subu
        43  mul(three)
        44  mulu(three)
        45  div(three)
        46  divu(three)
        47  xor
        48  xoru
        49  rem
        50  remu
        51  seq
        52  sge
        53  sgt
        54  sle
        55  slt
        56  sne
        57  beq
        58  bne
        59  ble
        60  bge
        61  bgt
        62  blt
        63  move
        64 _label
        65 #


        type that need wb:
        .ascii
        .asciiz
        .byte
        .half
        .word
        add addu addiu
        sub subu
        mul mulu
        div divu
        xor xoru
        neg negu
        rem remu
        li
        seq sge sgt sle slt sne
        la lb lh lw?
        sb sh sw?
        move mfhi mflo

    */

int StringToInt(string &s)
{
    bool flag = false;
    if(s[0] == '-')
    {
        flag = true;
        s = s.substr(1);
    }
    int len = s.size();
    int ten = 1;
    int ans = 0;
    for(int i = 1; i <= len - 1; ++i)
    {
        char n = s[len - i];
        ans += (n - '0') * ten;
        ten *= 10;
    }
    if(s[0] == '-')
        ans*=-1;
    else
        ans+= (s[0] - '0') *ten;
    if(flag)
        return -ans;
    else
        return ans;
}

void erase_space(string &s) // erase ' ' and '\t'
{
    int pos = 0;
    int xpos = s.find('"');
    int ypos;
    bool flag = false;
    if(xpos != string::npos)
    {
        flag = true;
    }
    if( !s.empty())
    {
        while( (pos = s.find(' ',pos)) != string::npos)
        {
            if(flag)
            {
                xpos = s.find('"');
                ypos = s.rfind('"');
                if( pos > xpos && pos < ypos)
                    pos = ypos;
                else
                    s.erase(pos,1);
            }
            else
                s.erase(pos,1);
        }
    }
    pos = 0;
    if( !s.empty())
    {
        while( (pos = s.find('\t',pos)) != string::npos)
        {
            if(flag)
            {
                xpos = s.find('"');
                ypos = s.rfind('"');
                if( pos > xpos && pos < ypos)
                    pos = ypos;
                else
                    s.erase(pos,1);
            }
            else
                s.erase(pos,1);
        }
    }
}

int trans_reg(string &s)
{
    if(s[0] != '$')
        return -1;

    if(s[1] >= 'a' && s[1] <= 'z')
    {
        if(s[1] == 'z')
            return 0;
        if(s[2] == 't')
            return 1;
        if(s[1] == 's' && s[2] == 'p' )
            return 29;
        if(s[1] == 'f')
            return 30;
        if(s[1] == 'r')
            return 31;

        if(s[1] == 'v')
            return s[2] - '0' + 2;
        if(s[1] == 'a')
            return s[2] - '0' + 4;  // $at has been judged
        if(s[1] == 's')
            return s[2] - '0' + 16;
        if(s[1] == 'k')
            return s[2] - '0' + 26; //k0 k1
        else
        {
            if(s[2] <= '7')
                return s[2] - '0' + 8;
            else
                return s[2] - '0' + 24;
        }
    }
    else
    {
        int len = s.size();
        if(len == 2)
        {
            return s[1] - '0';
        }
        else
        {
            return (s[1] - '0') * 10 + s[2] - '0';
        }
    }
}


void binary_mul(__int64  a, __int64  b)
{
    bool flaga=(a<0);
    if(a<0)
        a=-a;
    bool flagb=(b<0);
    if(b<0)
        b=-b;
    __int64  hi = 0;
    __int64  lo = 0;
    map<int,int> bits;
    for(int i=0; i<40; i++)
    {
        bits.insert(pair<int,int>(1<<i,i));
    }
    while(b > 0)
    {
        int bitshift=bits[b&~(b-1)];
        lo+=(a<<bitshift);
        b&=b-1;
    }
    if(lo>>32 >= 0)
    {
        hi = lo>>32;
        lo = lo % 2147483648; ///?
        if(flaga^flagb)
            hi = -hi;
        reg[32] = hi;
        reg[33] = lo;
    }
    else
    {
        hi = 0;
        if(flaga^flagb)
            lo = -lo;
        reg[32] = hi;
        reg[33] = lo;
    }
}



void nop()
{
    //do nothing
}

bool syscall()
{
    if (reg[2] == 1)
    {
        cout<<reg[4]<<endl;
        return 0;
    }
    if (reg[2] == 4)
    {
        int i = reg[4];
        while( mem[i] !='\0' )
        {
            cout<<mem[i];
            i++;
        }
        return 0;
    }
    if (reg[2] == 5)
    {
        int i;
        cin>>i;
        reg[2] = i;
        return 0;
    }
    if (reg[2] == 8)
    {
        mem_pos = reg[4];
        string s;
        cin>>s;

        int i = 0;
        while(s[i] != '\0')
        {
            mem[mem_pos++] = s[i++];  ///'\0'?
        }
        reg[5] = i + 1;
        cout<<reg[5]<<endl;
        system("pause");
        return 0;
    }
    if (reg[2] == 9)
    {
        for(int i = 0; i < 4; i++)
        {
            if( (mem_pos + i) % 4 == 0 )
            {
                mem_pos += i;
                reg[2] = mem_pos;
                break;
            }
        }
        mem_pos+=reg[4];
        return 0;
    }
    if (reg[2] == 10)
    {
        return 1;
    }
    if (reg[2] == 17)
    {
        return 1;
    }

    return 0;
}




int main(int argc, int **argv)
{
    fstream fin("a.txt");
    string now;
    while(getline(fin,now))
    {

        //cout<<"this line:"<<now<<endl;
        erase_space(now);
        if(now.size())
        {
            mapnum++;
            mm[mapnum] = now;
            if(!main_line && now == "main:")
            {
                main_line = mapnum + 1;
                continue;
            }
            if(now[0] == '_')
            {
                line[mapnum].type = 64;
                line[mapnum].num[0] = label_line_num;
                label_line[label_line_num++] = mapnum;
                continue;
            }
            if(now == "nop")
            {
                line[mapnum].type = 1;
                continue;
            }
            if(now == "syscall")
            {
                line[mapnum].type = 2;
                continue;
            }
            if(now == ".data")
            {
                line[mapnum].type = 3;
                continue;
            }
            if(now == ".text")
            {
                line[mapnum].type = 4;
                continue;
            }
            if(now[0] == '#')
            {
                line[mapnum].type = 65;
                continue;
            }

            if(now[0] == '.')
            {
                if(now[1] == 'a')
                {
                    string sub;
                    if(now[2] == 'l') ///.align
                    {
                        sub = now.substr(6);
                        line[mapnum].num[0] = StringToInt(sub);
                        line[mapnum].type = 5;
                        continue;
                    }
                    if(now[6] == 'z') ///.asciiz    没有前面那个“
                    {

                        sub = now.substr(8);
                        line[mapnum].num[0] = re_label_num;
                        re_label_line[re_label_num] = mapnum;
                        re_label[re_label_num++] = sub;
                        line[mapnum].type = 7;
                        continue;
                    }
                    else ///.ascii
                    {
                        sub = now.substr(7);
                        line[mapnum].num[0] = re_label_num;
                        re_label_line[re_label_num] = mapnum;
                        re_label[re_label_num++] = sub;


                        line[mapnum].type = 6;
                        continue;
                    }
                }
                if(now[1] == 's')     ///.space
                {
                    string sub = now.substr(6);
                    line[mapnum].num[0] = StringToInt(sub);
                    line[mapnum].type = 11;
                    continue;
                }
                string sub = now.substr(5);
                line[mapnum].num[0] = re_label_num;
                re_label_line[re_label_num] = mapnum;
                re_label[re_label_num++] = sub;
                if(now[1] == 'b') ///.byte  bool?
                {
                    line[mapnum].type = 8;
                    continue;
                }
                if(now[1] == 'h') ///.half  难以确定这个是否是对的 short?
                {
                    line[mapnum].type = 9;
                    continue;
                }
                if(now[1] == 'w') ///.word
                {
                    line[mapnum].type = 10;
                    continue;
                }
            }


            if(now[0] == 'b' && now[1] =='_')
            {
                line[mapnum].num[0] = re_label_num;
                re_label_line[re_label_num] = mapnum;
                re_label[re_label_num++] = now.substr(1);
                line[mapnum].type = 12;
                continue;
            }
            if(now[0] == 'j' && now[1] =='_')
            {
                line[mapnum].num[0] = re_label_num;
                re_label_line[re_label_num] = mapnum;
                re_label[re_label_num++] = now.substr(1);
                line[mapnum].type = 12;
                continue;
            }
            if(now[0] == 'j')
            {
                if(now[1] == 'r')
                {
                    string sub = now.substr(2);
                    line[mapnum].num[0] = trans_reg(sub);
                    line[mapnum].type = 13;
                    continue;
                }
                else if(now[4] == 'r')  ///jalr
                {
                    string sub = now.substr(4);
                    line[mapnum].num[0] = trans_reg(sub);
                    line[mapnum].type = 15;
                    continue;
                }
                else    ///jal
                {
                    string sub = now.substr(3);
                    line[mapnum].num[0] = re_label_num;
                    re_label_line[re_label_num] = mapnum;
                    re_label[re_label_num++] = sub;
                    line[mapnum].type = 14;
                    continue;
                }
            }

            if(now[0] == 'm' && now[2] == 'h' ) ///mfhi
            {
                string sub = now.substr(4);
                line[mapnum].num[0] = trans_reg(sub);
                line[mapnum].type = 16;
                continue;
            }
            if(now[0] == 'm' && now[1] == 'f' && now[2] == 'l' )
            {
                string sub = now.substr(4);
                line[mapnum].num[0] = trans_reg(sub);
                line[mapnum].type = 17;
                continue;
            }
            // one canshu
            int first = now.find(',');
            int second = now.rfind(',');



            if(first == second)
            {
                string pre = now.substr(0,2);
                string sub2 = now.substr(first + 1);
                string sub1;
                if(pre == "li" )
                {
                    sub1 = now.substr(2,first - 2);

                    line[mapnum].num[0] = trans_reg(sub1);
                    line[mapnum].num[1] = StringToInt(sub2);

                    line[mapnum].type = 24;
                    continue;
                }
                if(pre == "mu")
                {
                    if(now[3] == 'u')
                    {
                        sub1 = now.substr(4,first - 4);
                        line[mapnum].type = 19;
                    }
                    else
                    {
                        sub1 = now.substr(3,first - 3);
                        line[mapnum].type = 18;
                    }
                    line[mapnum].num[0] = trans_reg(sub1);
                    int two = trans_reg(sub2);
                    if(two == -1)
                    {
                        line[mapnum].num[1] = StringToInt(sub2);
                        line[mapnum].isnum = true;
                    }
                    else
                        line[mapnum].num[1] = two;

                    continue;

                }
                if(pre == "di")
                {

                    if(now[3] == 'u')
                    {
                        sub1 = now.substr(4,first - 4);
                        line[mapnum].type = 21;
                    }
                    else
                    {
                        sub1 = now.substr(3,first - 3);
                        line[mapnum].type = 20;
                    }
                    line[mapnum].num[0] = trans_reg(sub1);
                    int two = trans_reg(sub2);
                    if(two == -1)
                    {
                        line[mapnum].num[1] = StringToInt(sub2);
                        line[mapnum].isnum = true;
                    }
                    else
                        line[mapnum].num[1] = two;
                    continue;
                }
                if(pre == "ne")
                {
                    if(now[3] == 'u')
                    {
                        sub1 = now.substr(4,first - 4);
                        line[mapnum].type = 23;
                    }
                    else
                    {
                        sub1 = now.substr(3,first - 3);
                        line[mapnum].type = 22;
                    }
                    line[mapnum].num[0] = trans_reg(sub1);
                    line[mapnum].num[1] = trans_reg(sub2);
                    continue;
                }

                if(now[0] == 'b')
                {
                    pre = now.substr(1,2);
                    sub1 = now.substr(4,first - 4);
                    line[mapnum].num[0] = trans_reg(sub1);
                    line[mapnum].num[2] = re_label_num;
                    re_label_line[re_label_num] = mapnum;
                    re_label[re_label_num++] = sub2;

                    if(pre == "eq")
                        line[mapnum].type = 25;
                    else if(pre == "ne")
                        line[mapnum].type = 26;
                    else if(pre == "le")
                        line[mapnum].type = 27;
                    else if(pre == "ge")
                        line[mapnum].type = 28;
                    else if(pre == "gt")
                        line[mapnum].type = 29;
                    else if(pre == "lt")
                        line[mapnum].type = 30;
                    continue;
                }

                if(pre == "mo") ///move
                {
                    sub1 = now.substr(4,first - 4);
                    line[mapnum].num[0] = trans_reg(sub1);
                    line[mapnum].num[1] = trans_reg(sub2);
                    line[mapnum].type = 63;
                    continue;
                }
                //save & load
                sub1 = now.substr(2,first - 2);
                line[mapnum].num[0] = trans_reg(sub1);
                if(sub2[0] == '_')
                {
                    line[mapnum].num[1] = re_label_num;
                    re_label_line[re_label_num] = mapnum;
                    re_label[re_label_num++] = sub2;
                }
                else
                {
                    int kuo = sub2.find('(');
                    string sub3 = sub2.substr(kuo+1,1);
                    int pos = kuo + 2;
                    while(sub2[pos] != ')')
                    {
                        sub3+=sub2[pos++];
                    }
                    sub2 = sub2.substr(0,kuo);
                    int two = StringToInt(sub2);
                    int three = trans_reg(sub3);
                    line[mapnum].num[1] = two;
                    line[mapnum].num[2] = three;
                    line[mapnum].isnum = true;
                }

                if(pre == "la")
                {
                    line[mapnum].type = 31;
                    continue;
                }
                if(pre == "lb")
                {
                    line[mapnum].type = 32;
                    continue;
                }
                if(pre == "lh")
                {
                    line[mapnum].type = 33;
                    continue;
                }
                if(pre == "lw")
                {
                    line[mapnum].type = 34;
                    continue;
                }
                if(pre == "sb")
                {
                    line[mapnum].type = 35;
                    continue;
                }
                if(pre == "sh")
                {
                    line[mapnum].type = 36;
                    continue;
                }
                if(pre == "sw")
                {
                    line[mapnum].type = 37;
                    continue;
                }
            }


            // two canshu
            else
            {

                string pre = now.substr(0,3);
                string sub1;
                string sub2 = now.substr(first + 1,second - first - 1);
                string sub3 = now.substr(second + 1);

                line[mapnum].num[1] = trans_reg(sub2);

                if(pre == "add")        //add addu addiu
                {
                    if(now[3] == 'u' || now[3] =='i')   //addu
                    {
                        line[mapnum].type = 39;
                        sub1 = now.substr(4,first - 4);
                    }
                    else
                    {
                        line[mapnum].type = 38;
                        sub1 = now.substr(3,first - 3);
                    }
                }
                else if(pre == "sub")
                {
                    if(now[3] == 'u')
                    {
                        line[mapnum].type = 42;
                        sub1 = now.substr(4,first - 4);
                    }
                    else
                    {
                        line[mapnum].type = 41;
                        sub1 = now.substr(3,first - 3);
                    }
                }
                else if(pre == "mul")
                {
                    if(now[3] == 'u')
                    {
                        line[mapnum].type = 44;
                        sub1 = now.substr(4,first - 4);
                    }
                    else
                    {
                        line[mapnum].type = 43;
                        sub1 = now.substr(3,first - 3);
                    }
                }
                else if(pre == "div")
                {
                    if(now[3] == 'u')
                    {
                        line[mapnum].type = 46;
                        sub1 = now.substr(4,first - 4);
                    }
                    else
                    {
                        line[mapnum].type = 45;
                        sub1 = now.substr(3,first - 3);
                    }
                }
                else if(pre == "xor")
                {
                    if(now[3] == 'u')
                    {
                        line[mapnum].type = 48;
                        sub1 = now.substr(4,first - 4);
                    }
                    else
                    {
                        line[mapnum].type = 47;
                        sub1 = now.substr(3,first - 3);
                    }
                }
                else if(pre == "rem")
                {
                    if(now[3] == 'u')
                    {
                        line[mapnum].type = 50;
                        sub1 = now.substr(4,first - 4);
                    }
                    else
                    {
                        line[mapnum].type = 49;
                        sub1 = now.substr(3,first - 3);
                    }
                }
                if(line[mapnum].type)
                {
                    line[mapnum].num[0] = trans_reg(sub1);
                    int three = trans_reg(sub3);

                    if(three != -1)
                        line[mapnum].num[2] = three;
                    else
                    {
                        line[mapnum].num[2] = StringToInt(sub3);
                        line[mapnum].isnum = true;
                    }
                    continue;
                }
                //之后均为 三个字母的指令
                sub1 = now.substr(3,first - 3);
                line[mapnum].num[0] = trans_reg(sub1);

                if(sub3[0] != '_')
                {
                    int three = trans_reg(sub3);
                    if(three != -1)
                        line[mapnum].num[2] = three;
                    else
                    {
                        line[mapnum].num[2] = StringToInt(sub3);
                        line[mapnum].isnum = true;
                    }
                    if(pre == "seq")
                        line[mapnum].type = 51;
                    if(pre == "sge")
                        line[mapnum].type = 52;
                    if(pre == "sgt")
                        line[mapnum].type = 53;
                    if(pre == "sle")
                        line[mapnum].type = 54;
                    if(pre == "slt")
                        line[mapnum].type = 55;
                    if(pre == "sne")
                        line[mapnum].type = 56;
                    continue;
                }
                else
                {
                    if(line[mapnum].num[1] == -1)
                    {
                        line[mapnum].num[1] = StringToInt(sub2);
                        line[mapnum].isnum = true;
                    }
                    if(pre == "beq")
                        line[mapnum].type = 57;
                    else if(pre == "bne")
                        line[mapnum].type = 58;
                    else if(pre == "ble")
                        line[mapnum].type = 59;
                    else if(pre == "bge")
                        line[mapnum].type = 60;
                    else if(pre == "bgt")
                        line[mapnum].type = 61;
                    else if(pre == "blt")
                        line[mapnum].type = 62;

                    re_label_line[re_label_num] = mapnum;
                    re_label[re_label_num++] = sub3;
                    continue;
                }
            }
        }
    }

    reg[29] = 4194304 - 1; //ran er ken ding bu dui
//初始化

    for(int i = 0; i < re_label_num; i++)
    {
        cout<<re_label[i]<<endl;
        if(re_label[i][0] == '_')
        {
            string s = re_label[i];
            s+=':';
            for(int j = 0; j < label_line_num; j++)
            {
                if(s == mm[label_line[j]])
                {
                    line[re_label_line[i]].num[2] =  j; ///这里有改动注意
                    break;
                }
            }
        }
        else line[re_label_line[i]].num[2] = i;
    }
    //system("pause");


    for(int i = 1; i < mapnum + 1; i++)
    {
        cout<<mm[i]<<endl;
        cout<<line[i].type<<' '<<line[i].isnum<<' '<<line[i].num[0]<<' '<<line[i].num[1]<<' '<<line[i].num[2]<<endl;
    }




    xxx water[5];
    int water_wb[5]; ///缓存
    //int hi,lo;
    int current_line = 1;
    int loop = 0;
    int ins_fetch, ins_decode,execution,mem_access,write_back;


    while(true)
    {



        ins_fetch   =  loop % 5;
        ins_decode  = (loop + 4) % 5;
        execution   = (loop + 3) % 5;
        mem_access  = (loop + 2) % 5;
        write_back  = (loop + 1) % 5;
        loop++;
        if(current_line >mapnum + 5) break;

        cout<<"line: "<<current_line<<endl;
        cout<<mm[current_line - 1]<<endl;
        cout<<"mempos: "<<mem_pos<<endl;
        cout<<water[ins_decode].type<<' '<<water[ins_decode].isnum<<' '<<water[ins_decode].num[0]<<' '<<water[ins_decode].num[1]<<' '<<water[ins_decode].num[2]<<endl;
        cout<<water[execution].type<<' '<<water[execution].isnum<<' '<<water[execution].num[0]<<' '<<water[execution].num[1]<<' '<<water[execution].num[2]<<endl;
        cout<<water[mem_access].type<<' '<<water[mem_access].isnum<<' '<<water[mem_access].num[0]<<' '<<water[mem_access].num[1]<<' '<<water[mem_access].num[2]<<endl;
        cout<<water[write_back].type<<' '<<water[write_back].isnum<<' '<<water[write_back].num[0]<<' '<<water[write_back].num[1]<<' '<<water[write_back].num[2]<<endl;
        cout<<water[ins_fetch].type<<' '<<water[ins_fetch].isnum<<' '<<water[ins_fetch].num[0]<<' '<<water[ins_fetch].num[1]<<' '<<water[ins_fetch].num[2]<<endl;


        system("pause");

        //write

        //wb 16-17 22-24 38-56 63   2  14-15
        if(water[write_back].type == 16 ||  water[write_back].type ==17)
            reg[ water[write_back].num[0] ] = water_wb[write_back];
        else if(water[write_back].type >= 22 &&  water[write_back].type <=24)
            reg[ water[write_back].num[0] ] = water_wb[write_back];
        else if(water[write_back].type >= 38 &&  water[write_back].type <=56)
            reg[ water[write_back].num[0] ] = water_wb[write_back];
        else if(water[write_back].type == 63)
            reg[ water[write_back].num[0] ] = water_wb[write_back];
        else if(water[write_back].type == 2 ) ///syscall 的缓存只有v0或者a0
        {
        //cout<<reg[2]<<' '<<reg[4]<<endl;
         bool flag = syscall();
         if(flag) break;
        }
        else if(water[write_back].type == 14 || water[write_back].type == 15)
            reg[31] = water_wb[write_back];


        //load &save

        if(water[mem_access].type == 5)
        {
            int x = pow(2,water[mem_access].num[0]);
            cout<<"x:"<<x<<endl;
            cout<<mem_pos<<endl;
            int i = 1;
            while(x * i < mem_pos)
                i++;
            mem_pos = i  * x;
            cout<<mem_pos<<endl;
            system("pause");
        }
        if(water[mem_access].type == 11)
        {
            mem_pos += water[mem_access].num[0];
        }
        if(water[mem_access].type == 6 || water[mem_access].type == 7)
        {
            int pos = 0;
            string now = re_label[water[mem_access].num[2]];
            //cout<<"now:"<<now<<endl;
                while(now[pos] != '\"')
                {
                   // cout<<"!!!: "<<mem_pos<<endl;
                    if(now[pos] == '\\')
                    {
                        pos++;
                        if(now[pos] == 'a')
                            mem[mem_pos++] = 7;
                        else  if(now[pos] == 'b')
                            mem[mem_pos++] = 8;
                        else  if(now[pos] == 'f')
                            mem[mem_pos++] = 12;
                        else  if(now[pos] == 'n')
                            mem[mem_pos++] = 10;
                        else  if(now[pos] == 't')
                            mem[mem_pos++] = 13;
                        else  if(now[pos] == 'v')
                            mem[mem_pos++] = 11;
                        else  if(now[pos] == '\\')
                            mem[mem_pos++] = 92;
                        else  if(now[pos] == '?')
                            mem[mem_pos++] = 63;
                        else  if(now[pos] == '\'')
                            mem[mem_pos++] = 39;
                        else  if(now[pos] == '\"')
                            mem[mem_pos++] = 34;
                        else  if(now[pos] == '0')
                            mem[mem_pos++] = 0;
                        pos++;
                    }
                    else
                        mem[mem_pos++] = now[pos++];
                }
                if(water[mem_access].type == 7)
                    mem[mem_pos++] = '\0';
        }
        if(water[mem_access].type == 8)
        {
            int dpos = 0;
            int pos = -1;///..???
            string now = re_label[water[mem_access].num[0]];
                while(now.find(',',dpos) != string::npos)
                {
                    dpos = now.find(',',dpos);
                    string sub = now.substr(pos + 1,dpos - pos - 1);
                    int x = StringToInt(sub);
                    mem[mem_pos++] = char(x);
                    pos = dpos;
                    dpos++;
                }
                pos++;
                string sub = now.substr(pos);
                int x = StringToInt(sub);
                mem[mem_pos++] = char(x);
        }
        if(water[mem_access].type == 9)
        {
            int dpos = 0;
            int pos = -1;
            string now = re_label[water[mem_access].num[0]];
                while(now.find(',',dpos) != string::npos)
                {
                    dpos = now.find(',',dpos);
                    string sub = now.substr(pos + 1,dpos - pos - 1);
                    int x = StringToInt(sub);
                    mem[mem_pos++] = char(x>>8);
                    mem[mem_pos++] = char(x);
                    pos = dpos;
                    dpos++;
                }
                pos++;
                string sub = now.substr(pos);
                int x = StringToInt(sub);
                mem[mem_pos++] = char(x>>8);
                mem[mem_pos++] = char(x);
        }
        if(water[mem_access].type == 10)
        {
            int dpos = 0;
            int pos = -1;
            string now = re_label[water[mem_access].num[0]];
                while(now.find(',',dpos) != string::npos)
                {
                    dpos = now.find(',',dpos);
                    string sub = now.substr(pos + 1,dpos - pos - 1);
                    int x = StringToInt(sub);
                    mem[mem_pos++] = char(x>>24);
                    mem[mem_pos++] = char(x>>16);
                    mem[mem_pos++] = char(x>>8);
                    mem[mem_pos++] = char(x);
                    pos = dpos;
                    dpos++;
                }
                pos++;
                string sub = now.substr(pos);
                int x = StringToInt(sub);
                mem[mem_pos++] = char(x>>24);
                mem[mem_pos++] = char(x>>16);
                mem[mem_pos++] = char(x>>8);
                mem[mem_pos++] = char(x);
        }
        if(water[mem_access].type == 31)
        {
            reg[water[mem_access].num[0]] = water[mem_access].num[1];
        }
        else  if(water[mem_access].type == 32)
        {
            reg[water[mem_access].num[0]] = int(mem[water[mem_access].num[1]]);
        }
        else  if(water[mem_access].type == 33)
        {
            reg[water[mem_access].num[0]] = int(mem[water[mem_access].num[1]] <<8) + int(mem[water[mem_access].num[1] + 1]);
        }
        else  if(water[mem_access].type == 34)
        {
            reg[water[mem_access].num[0]] = int(mem[water[mem_access].num[1]] <<24) + int(mem[water[mem_access].num[1] + 1] <<16) + int(mem[water[mem_access].num[1] + 2] <<8) + int(mem[water[mem_access].num[1] + 3]);
        }
        else  if(water[mem_access].type == 35)
        {
           mem[water[mem_access].num[1]] = char(reg[water[mem_access].num[0]]);
        }
        else  if(water[mem_access].type == 36)
        {
           mem[water[mem_access].num[1]] = char(reg[water[mem_access].num[0]] >>8 );
           mem[water[mem_access].num[1] + 1] = char(reg[water[mem_access].num[0]]);
        }
        else  if(water[mem_access].type == 37)
        {
           mem[water[mem_access].num[1]] = char(reg[water[mem_access].num[0]]>>24);
           mem[water[mem_access].num[1] + 1] = char(reg[water[mem_access].num[0]]>>16);
           mem[water[mem_access].num[1] + 2] = char(reg[water[mem_access].num[0]]>>8);
           mem[water[mem_access].num[1] + 3] = char(reg[water[mem_access].num[0]]);
        }

        //read
        //ins_fetch
        water[ins_fetch] = line[current_line];

        //ins_decode
        if(water[ins_decode].type == 2) ///syscall
        {
        }
        else if(water[ins_decode].type == 12)
        {
            water[ins_decode].num[2] = label_line[water[ins_decode].num[2]];
        }
        else if(water[ins_decode].type == 13 || water[ins_decode].type == 15)
        {
            water[ins_decode].num[2] = reg[water[ins_decode].num[0]];
        }
        else if(water[ins_decode].type == 14)
        {
            water[ins_decode].num[2] = label_line[water[ins_decode].num[2]];
        }
        else if(water[ins_decode].type == 16)
        {
            water[ins_decode].num[0] = reg[32];
        }
        else if(water[ins_decode].type == 17)
        {
            water[ins_decode].num[0] = reg[33];
        }
        else if(water[ins_decode].type >= 18 && water[ins_decode].type <= 21)
        {
            water[ins_decode].num[0] = reg[water[ins_decode].num[0]];
            if(water[ins_decode].isnum == 0)
                water[ins_decode].num[1] = reg[water[ins_decode].num[1]];
        }
        else if(water[ins_decode].type == 22 || water[ins_decode].type == 23)
        {
            if(water[ins_decode].isnum == 0)
                water[ins_decode].num[1] = reg[water[ins_decode].num[1]];
        }
        else if(water[ins_decode].type >= 25 && water[ins_decode].type <= 30)
        {
            water[ins_decode].num[0] = reg[water[ins_decode].num[0]];
            water[ins_decode].num[2] = label_line[water[ins_decode].num[2]];
        }
        else if(water[ins_decode].type >= 31 && water[ins_decode].type <= 37)
        {
            if(water[ins_decode].isnum == 0)
            {
                water[ins_decode].num[1] = label_add[water[ins_decode].num[2]];
            }
            else  water[ins_decode].num[1] =  water[ins_decode].num[1] + reg[ water[ins_decode].num[2] ];
        }
        else if(water[ins_decode].type >= 38 && water[ins_decode].type <= 56)
        {
            water[ins_decode].num[1] = reg[water[ins_decode].num[1]];
            if(water[ins_decode].isnum == 0)
            {
                water[ins_decode].num[2] = reg[water[ins_decode].num[2]];
            }
        }
        else if(water[ins_decode].type >= 57 && water[ins_decode].type <= 62)
        {
            water[ins_decode].num[0] = reg[water[ins_decode].num[0]];
            if(water[ins_decode].isnum == 0)
            {
                water[ins_decode].num[1] = reg[water[ins_decode].num[1]];
            }
            water[ins_decode].num[2] = label_line[water[ins_decode].num[2]];
        }
        else if(water[ins_decode].type == 63)
        {
            water[ins_decode].num[1] = reg[water[ins_decode].num[1]];
        }




        current_line++;

        //execution
        if(water[execution].type == 0) {continue;}
        if(water[execution].type == 1 || water[execution].type == 3 || water[execution].type == 65)
        {continue;}
        if(water[execution].type >=5 && water[execution].type <= 11){continue;}
        if(water[execution].type >=31 && water[execution].type <= 37){continue;}
        if(water[execution].type == 4)
        {
            water[ins_decode].type = 0;
            current_line = main_line;
            water[ins_fetch] = line[current_line];
            current_line++;
            continue;
        }
        if(water[execution].type ==12 || water[execution].type ==13)
        {
            cout<<"change\n";
            system("pause");
            water[ins_decode].type = 0;
            current_line = water[execution].num[2];
            water[ins_fetch] = line[current_line];
            current_line++;
            continue;
        }
        if(water[execution].type ==14 || water[execution].type ==15)
        {
            cout<<"change\n";
            system("pause");
            water_wb[execution] = current_line - 1;
            water[ins_decode].type = 0;
            current_line = water[execution].num[2];
            water[ins_fetch] = line[current_line];
            current_line++;
            continue;
        }
        if(water[execution].type ==16)
        {
            water_wb[execution] = reg[32];
            continue;
        }
        if(water[execution].type ==17)
        {
            water_wb[execution] = reg[33];
            continue;
        }
        if(water[execution].type ==18)
        {
            binary_mul( water[execution].num[0], water[execution].num[1] );
            continue;
        }
        if(water[execution].type ==19)
        {
            binary_mul( abs(water[execution].num[0]), abs(water[execution].num[1]) );
            continue;
        }
        if(water[execution].type == 20)
        {
            reg[32] = water[execution].num[0] % water[execution].num[1];
            reg[33] = water[execution].num[0] / water[execution].num[1];
            continue;
        }
        if(water[execution].type == 21)
        {
            reg[32] = abs(water[execution].num[0] % water[execution].num[1]);
            reg[33] = abs(water[execution].num[0] / water[execution].num[1]);
            continue;
        }
        if(water[execution].type == 22)
        {
            water_wb[execution] = -water[execution].num[1];
            continue;
        }
        if(water[execution].type == 23)
        {
            water_wb[execution] = ~water[execution].num[1];
            continue;
        }
        if(water[execution].type == 24)
        {
            water_wb[execution] = water[execution].num[1];
            continue;
        }
        if(water[execution].type == 38)
        {
            water_wb[execution] = water[execution].num[1] + water[execution].num[2];
            continue;
        }
        if(water[execution].type == 39 || water[execution].type == 40)
        {
            water_wb[execution] = abs(water[execution].num[1] + water[execution].num[2]);
            continue;
        }
        if(water[execution].type == 41)
        {
            water_wb[execution] = water[execution].num[1] - water[execution].num[2];
            continue;
        }
        if(water[execution].type == 42)
        {
            water_wb[execution] = abs(water[execution].num[1] - water[execution].num[2]);
            continue;
        }
        if(water[execution].type == 43)
        {
            water_wb[execution] = water[execution].num[1] * water[execution].num[2];
            continue;
        }
        if(water[execution].type == 44)
        {
            water_wb[execution] = abs(water[execution].num[1] * water[execution].num[2]);
            continue;
        }
        if(water[execution].type == 45)
        {
            water_wb[execution] = water[execution].num[1] / water[execution].num[2];
            continue;
        }
        if(water[execution].type == 46)
        {
            water_wb[execution] = abs(water[execution].num[1] / water[execution].num[2]);
            continue;
        }
        if(water[execution].type == 47)
        {
            water_wb[execution] = water[execution].num[1] ^ water[execution].num[2];
            continue;
        }
        if(water[execution].type == 48)
        {
            water_wb[execution] = abs(water[execution].num[1] ^ water[execution].num[2]);
            continue;
        }
        if(water[execution].type == 49)
        {
            water_wb[execution] = water[execution].num[1] % water[execution].num[2];
            continue;
        }
        if(water[execution].type == 50)
        {
            water_wb[execution] = abs(water[execution].num[1] % water[execution].num[2]);
            continue;
        }
        if(water[execution].type == 51)
        {
            water_wb[execution] = (water[execution].num[1] == water[execution].num[2]);
            continue;
        }
        if(water[execution].type == 52)
        {
            water_wb[execution] = (water[execution].num[1] >= water[execution].num[2]);
            continue;
        }
        if(water[execution].type == 53)
        {
            water_wb[execution] = (water[execution].num[1] > water[execution].num[2]);
            continue;
        }
        if(water[execution].type == 54)
        {
            water_wb[execution] = (water[execution].num[1] <= water[execution].num[2]);
            continue;
        }
        if(water[execution].type == 55)
        {
            water_wb[execution] = (water[execution].num[1] < water[execution].num[2]);
            continue;
        }
        if(water[execution].type == 56)
        {
            water_wb[execution] = (water[execution].num[1] != water[execution].num[2]);
            continue;
        }
        if(water[execution].type == 63)
        {
            water_wb[execution] = water[execution].num[1];
            continue;
        }
        if(water[execution].type == 64)
        {
            label_add[water[execution].num[0]] = mem_pos;
            continue;
        }
        bool flag = false;
        if(water[execution].type == 25 && water[execution].num[0] == 0)
            flag = true;
        if(water[execution].type == 26 && water[execution].num[0] != 0)
            flag = true;
        if(water[execution].type == 27 && water[execution].num[0] <= 0)
            flag = true;
        if(water[execution].type == 28 && water[execution].num[0] >= 0)
            flag = true;
        if(water[execution].type == 29 && water[execution].num[0] > 0)
            flag = true;
        if(water[execution].type == 30 && water[execution].num[0] < 0)
            flag = true;
        if(water[execution].type == 57 && water[execution].num[0] == water[execution].num[1])
            flag = true;
        if(water[execution].type == 58 && water[execution].num[0] != water[execution].num[1])
            flag = true;
        if(water[execution].type == 59 && water[execution].num[0] >= water[execution].num[1])
            flag = true;
        if(water[execution].type == 60 && water[execution].num[0] <= water[execution].num[1])
            flag = true;
        if(water[execution].type == 61 && water[execution].num[0] > water[execution].num[1])
            flag = true;
        if(water[execution].type == 57 && water[execution].num[0] < water[execution].num[1])
            flag = true;

        if(flag)
        {
            water[ins_decode].type = 0;
            current_line = water[execution].num[2];
            water[ins_fetch] = line[current_line];
            current_line++;
        }
    }




    for(int i = 0; i < 34; i++)
    {
        cout<<"$"<<i<<": "<<reg[i]<<endl;
    }





    return 0;
}
