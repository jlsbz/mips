#include <iostream>
#include <string>
#include <fstream>
#include <map>
#include <cmath>
#include <functional>
#include <string.h>
#include <sstream>

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


    xxx &operator=(const xxx &other)
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


void binary_mul(long long  a, long long  b)
{
    bool flaga=(a<0);
    if(a<0)
        a=-a;
    bool flagb=(b<0);
    if(b<0)
        b=-b;
    long long   hi = 0;
    long long  lo = 0;
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
        printf("%d",&reg[4]);
        return 0;
    }
    if (reg[2] == 4)
    {
        int i = reg[4];
        while( mem[i] !='\0' )
        {
           // cout<<mem[i];
            printf("%c",mem[i]);
            i++;
        }
        return 0;
    }
    if (reg[2] == 5)
    {
        int i;
        scanf("%d",&i);
        reg[2] = i;
        return 0;
    }
    if (reg[2] == 8)
    {
        mem_pos = reg[4];
        string s;
        getline(cin,s);
        int i = 0;
        while(s[i] != '\0')
        {
            mem[mem_pos++] = s[i++];  ///'\0'?
        }
        reg[5] = i + 1;
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




int main(int argc, char *argv[])
{
    ifstream fin(argv[1]);
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
        bool flag = false;
       // cout<<re_label[i]<<endl;
        if(re_label[i][0] == '_')
        {
            string s = re_label[i];
            s+=':';
            if(flag) continue;
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



    int current_line = 1;


    while(current_line < mapnum + 1)
    {
        int now = current_line;
       // cout<<"line: "<<current_line<<endl;
       // cout<<mm[current_line]<<endl;
        //cout<<line[current_line].type<<' '<<line[current_line].isnum<<' '<<line[current_line].num[0]<<' '<<line[current_line].num[1]<<' '<<line[current_line].num[2];
       // cout<<endl;
        //cout<<"mem_pos:"<<mem_pos<<endl;
        // for(int i = 0; i < 32; i++){cout<<"$"<<i<<": "<<reg[i]<<endl;}



       // system("pause");


        if(line[now].type == 0) break;


        current_line++;
        if(line[now].type == 1 || line[now].type == 3 || line[now].type == 65)
        {
            continue;
        }
        if(line[now].type == 2)
        {
            bool flag = syscall();
            if(flag) break;
            continue;
        }
        if(line[now].type == 4)
        {
            current_line = main_line;
            continue;
        }
        if(line[now].type == 5)
        {
            int x = pow(2,line[now].num[0]);
           // cout<<"x:"<<x<<endl;
            //cout<<mem_pos<<endl;
            int i = 1;
            while(x * i < mem_pos)
                i++;
            mem_pos = i * x;
           // cout<<mem_pos<<endl;
            //system("pause");
            continue;
        }
        if(line[now].type == 6 || line[now].type == 7)
        {
            int pos = 0;
            string s = re_label[line[now].num[2]];
                while(s[pos] != '\"')
                {
                    if(s[pos] == '\\')
                    {
                        pos++;
                        if(s[pos] == 'a')
                            mem[mem_pos++] = 7;
                        else  if(s[pos] == 'b')
                            mem[mem_pos++] = 8;
                        else  if(s[pos] == 'f')
                            mem[mem_pos++] = 12;
                        else  if(s[pos] == 'n')
                            mem[mem_pos++] = 10;
                        else  if(s[pos] == 't')
                            mem[mem_pos++] = 13;
                        else  if(s[pos] == 'v')
                            mem[mem_pos++] = 11;
                        else  if(s[pos] == '\\')
                            mem[mem_pos++] = 92;
                        else  if(s[pos] == '?')
                            mem[mem_pos++] = 63;
                        else  if(s[pos] == '\'')
                            mem[mem_pos++] = 39;
                        else  if(s[pos] == '\"')
                            mem[mem_pos++] = 34;
                        else  if(s[pos] == '0')
                            mem[mem_pos++] = 0;
                        pos++;
                    }
                    else
                        mem[mem_pos++] = s[pos++];
                }
                if(line[now].type == 7)
                    mem[mem_pos++] = '\0';
            continue;
        }
        if(line[now].type == 11)
        {
            mem_pos += line[now].num[0];
            continue;
        }
        if(line[now].type == 8)
        {
            int dpos = 0;
            int pos = -1;///..???
            string s = re_label[line[now].num[0]];
                while(s.find(',',dpos) != string::npos)
                {
                    dpos = s.find(',',dpos);
                    string sub = s.substr(pos + 1,dpos - pos - 1);
                    int x = StringToInt(sub);
                    mem[mem_pos++] = char(x);
                    pos = dpos;
                    dpos++;
                }
                pos++;
                string sub = s.substr(pos);
                int x = StringToInt(sub);
                mem[mem_pos++] = char(x);
            continue;
        }
        if(line[now].type == 9)
        {
            int dpos = 0;
            int pos = -1;
            string s = re_label[line[now].num[0]];
                while(s.find(',',dpos) != string::npos)
                {
                    dpos = s.find(',',dpos);
                    string sub = s.substr(pos + 1,dpos - pos - 1);
                    int x = StringToInt(sub);
                    mem[mem_pos++] = char(x>>8);
                    mem[mem_pos++] = char(x);
                    pos = dpos;
                    dpos++;
                }
                pos++;
                string sub = s.substr(pos);
                int x = StringToInt(sub);
                mem[mem_pos++] = char(x>>8);
                mem[mem_pos++] = char(x);
            continue;
        }
        if(line[now].type == 10)
        {
            int dpos = 0;
            int pos = -1;
            string s = re_label[line[now].num[0]];
                while(s.find(',',dpos) != string::npos)
                {
                    dpos = s.find(',',dpos);
                    string sub = s.substr(pos + 1,dpos - pos - 1);
                    int x = StringToInt(sub);
                    mem[mem_pos++] = char(x>>24);
                    mem[mem_pos++] = char(x>>16);
                    mem[mem_pos++] = char(x>>8);
                    mem[mem_pos++] = char(x);
                    pos = dpos;
                    dpos++;
                }
                pos++;
                string sub = s.substr(pos);
                int x = StringToInt(sub);
                mem[mem_pos++] = char(x>>24);
                mem[mem_pos++] = char(x>>16);
                mem[mem_pos++] = char(x>>8);
                mem[mem_pos++] = char(x);
            continue;
        }
        if(line[now].type == 12)
        {
            current_line = label_line[line[now].num[2]];
            continue;
        }
        if(line[now].type == 13)
        {
            current_line = reg[line[now].num[0]];
            continue;
        }
        if(line[now].type == 14)
        {
            reg[31] = current_line;
            current_line = label_line[line[now].num[2]];
            continue;
        }
        if(line[now].type == 15)
        {
            reg[31] = current_line;
            current_line = reg[line[now].num[0]];
            continue;
        }
        if(line[now].type == 16)
        {
            reg[line[now].num[0]] = reg[32];
            continue;
        }
        if(line[now].type == 17)
        {
            reg[line[now].num[0]] = reg[33];
            continue;
        }
        if(line[now].type == 18)
        {
            if(line[now].isnum == 0)
            binary_mul( reg[line[now].num[0]] , reg[line[now].num[1]]);
            else binary_mul( reg[line[now].num[0]] , line[now].num[1]);
            continue;
        }
        if(line[now].type == 19)
        {
            if(line[now].isnum == 0)
            binary_mul( abs( reg[line[now].num[0]]) , abs(reg[line[now].num[1]]) );
            else binary_mul( abs( reg[line[now].num[0]]) , abs(line[now].num[1]) );
            continue;
        }
        if(line[now].type == 20)
        {
            if(line[now].isnum == 0)
            {
                reg[32] = reg[line[now].num[0]] % reg[line[now].num[1]];
                reg[33] = reg[line[now].num[0]] / reg[line[now].num[1]];
                }
            else
            {
                reg[32] = reg[line[now].num[0]] % line[now].num[1];
                reg[33] = reg[line[now].num[0]] / line[now].num[1];
                }
            continue;
        }
        if(line[now].type == 21)
        {
            if(line[now].isnum == 0)
            {reg[32] = abs(reg[line[now].num[0]] % reg[line[now].num[0]]);
            reg[33] = abs(reg[line[now].num[0]] / reg[line[now].num[0]]);}
            else
            {reg[32] = abs(reg[line[now].num[0]] % line[now].num[1]);
            reg[33] = abs(reg[line[now].num[0]] / line[now].num[1]);}
            continue;
        }
        if(line[now].type == 22)
        {
            if(line[now].isnum == 0)
            reg[line[now].num[0]] = -reg[line[now].num[1]];
            else reg[line[now].num[0]] = -line[now].num[1];
            continue;
        }
        if(line[now].type == 23)
        {
            if(line[now].isnum == 0)
            reg[line[now].num[0]] = abs(~reg[line[now].num[1]]);
            else reg[line[now].num[0]] = abs(~line[now].num[1]);
            continue;
        }
        if(line[now].type == 24)
        {
            reg[line[now].num[0]] = line[now].num[1];
            continue;
        }
        if(line[now].type == 25 && reg[line[now].num[0]] == 0)
        {
            current_line = label_line[line[now].num[2]];
            continue;
        }
        if(line[now].type == 26 && reg[line[now].num[0]] != 0)
        {
            current_line = label_line[line[now].num[2]];
            continue;
        }
        if(line[now].type == 27 && reg[line[now].num[0]] <= 0)
        {
            current_line = label_line[line[now].num[2]];
            continue;
        }
        if(line[now].type == 28 && reg[line[now].num[0]] >= 0)
        {
            current_line = label_line[line[now].num[2]];
            continue;
        }
        if(line[now].type == 29 && reg[line[now].num[0]] > 0)
        {
            current_line = label_line[line[now].num[2]];
            continue;
        }
        if(line[now].type == 30 && reg[line[now].num[0]] < 0)
        {
            current_line = label_line[line[now].num[2]];
            continue;
        }
        if(line[now].type >= 31 && line[now].type <=37)
        {
            int x;
            if(line[now].isnum == 0)
                x = label_add[line[now].num[2]];
            else x = line[now].num[1] + reg[line[now].num[2]];


            //cout<<"num: "<<line[now].num[1]<<endl;
            if(line[now].type == 31)
            {
                reg[line[now].num[0]] = x;
            }
            if(line[now].type == 32)
            {
                reg[line[now].num[0]] = int(mem[x]);
            }
            if(line[now].type == 33)
            {
                reg[line[now].num[0]] = int(mem[x] << 8 ) + int(mem[x + 1]);
            }
            if(line[now].type == 34)
            {
                reg[line[now].num[0]] = int(mem[x]<<24) + int(mem[x + 1]<<16) + int(mem[x + 2]<<8) + int(mem[x + 3]);
            }
            if(line[now].type == 35)
            {
                mem[x] = char(reg[line[now].num[0]]);
            }
            if(line[now].type == 36)
            {
                mem[x] = char(reg[line[now].num[0]]);
                mem[x + 1] = char(reg[line[now].num[0]] >> 8);
            }
            if(line[now].type == 37)
            {
                mem[x] = char(reg[line[now].num[0]]>>24);
                mem[x + 1] = char(reg[line[now].num[0]]>>16);
                mem[x + 2] = char(reg[line[now].num[0]]>>8);
                mem[x + 3] = char(reg[line[now].num[0]]);
            }
            continue;
        }
        if(line[now].type >= 38 && line[now].type <= 56)
        {
            int x;
            if(line[now].isnum == 0)
            {
                x = reg[line[now].num[2]];
            }
            else x = line[now].num[2];
          //  cout<<line[now].num[1]<<' '<<line[now].num[2]<<endl;

            if(line[now].type == 38)
                reg[line[now].num[0]] = reg[line[now].num[1]] + x;
            if(line[now].type == 39)
                reg[line[now].num[0]] = abs(reg[line[now].num[1]] + x);
            if(line[now].type == 40)
                reg[line[now].num[0]] = abs(reg[line[now].num[1]] + x);
            if(line[now].type == 41)
                reg[line[now].num[0]] = reg[line[now].num[1]] - x;
            if(line[now].type == 42)
                reg[line[now].num[0]] = abs(reg[line[now].num[1]] - x);
            if(line[now].type == 43)
                reg[line[now].num[0]] = reg[line[now].num[1]] * x;
            if(line[now].type == 44)
                reg[line[now].num[0]] = abs(reg[line[now].num[1]] * x);
            if(line[now].type == 45)
                reg[line[now].num[0]] = reg[line[now].num[1]] / x;
            if(line[now].type == 46)
                reg[line[now].num[0]] = abs(reg[line[now].num[1]] / x);
            if(line[now].type == 47)
                reg[line[now].num[0]] = reg[line[now].num[1]] ^ x;
            if(line[now].type == 48)
                reg[line[now].num[0]] = abs(reg[line[now].num[1]] ^ x);
            if(line[now].type == 49)
                reg[line[now].num[0]] = reg[line[now].num[1]] % x;
            if(line[now].type == 50)
                reg[line[now].num[0]] = abs(reg[line[now].num[1]] % x);
            if(line[now].type == 51)
                    reg[line[now].num[0]] = (reg[line[now].num[1]] == x);
            if(line[now].type == 52)
                    reg[line[now].num[0]] = (reg[line[now].num[1]] >= x);
            if(line[now].type == 53)
                    reg[line[now].num[0]] = (reg[line[now].num[1]] > x);
            if(line[now].type == 54)
                    reg[line[now].num[0]] = (reg[line[now].num[1]] <= x);
            if(line[now].type == 55)
                    reg[line[now].num[0]] = (reg[line[now].num[1]] < x);
            if(line[now].type == 56)
                    reg[line[now].num[0]] = (reg[line[now].num[1]] != x);

            continue;
        }
        if(line[now].type >= 57 && line[now].type <=62)
        {
            int x;
            if(line[now].isnum == 0)
            x = reg[line[now].num[1]];
            else x = line[now].num[1];
            if(line[now].type == 57 && reg[line[now].num[0]] == x)
                current_line = label_line[line[now].num[2]];
            if(line[now].type == 58 && reg[line[now].num[0]] != x)
                current_line = label_line[line[now].num[2]];
            if(line[now].type == 59 && reg[line[now].num[0]] <= x)
                current_line = label_line[line[now].num[2]];
            if(line[now].type == 60 &&reg[line[now].num[0]] >= x)
                {

                    current_line = label_line[line[now].num[2]];
                    }
            if(line[now].type == 61 && reg[line[now].num[0]] > x)
                current_line = label_line[line[now].num[2]];
            if(line[now].type == 62 && reg[line[now].num[0]] < x)
                current_line = label_line[line[now].num[2]];
            continue;
        }
        if(line[now].type == 63)
        {
            reg[line[now].num[0]] = reg[line[now].num[1]];
            continue;
        }
        if(line[now].type == 64)
        {
            label_add[line[now].num[0]] = mem_pos;
            continue;
        }


    }


    return 0;

}
