#include<stdio.h>
#include<stdint.h>
#include<inttypes.h>

char *itoa(uint32_t pid,char *str)
{
    int t=0;
    if(pid ==0)
    {
        str[t] = '0';
        t++;
        str[t] = '\0';
        return str;
    }
    while(pid>0)
    {
        str[t] = (pid%10)+'0';
        pid/=10;
        t++;
    }
    str[t] = '\0';
    //reverse the string
    int s=0,e=t-1;
    while(s<e)
    {
        char temp = str[e];
        str[e]=str[s];
        str[s]=temp;
        s++;
        e--;
    }
    return str;
}

int length_str(char *str)
{
    int length=0;
    while(str[length]!='\0')
    {
        length++;
    }
    return length;
}

int cmp_str(char *str1,char *str2)
{
    int i=0;
    if(length_str(str1)!=length_str(str2))
    {
        return -1;
    }
    while(str1[i]!='\0' && str2[i]!='\0')
    {
        if(str1[i]!=str2[i])
        {
            return -1;
        }
        i++;
    }
    return 0;
}

char *str_cat(char *str1,char *str2,char *final_str)
{
    final_str[0]='\0';
    int length1 = length_str(str1);
    int length2 = length_str(str2);
    int i=0,j=0;
    while(i<length1)
    {
        final_str[i] = str1[i];
        i++;
    }
    final_str[i++]=' ';
    while(j<length2)
    {
        final_str[i] = str2[j];
        i++;
        j++;
    }

    final_str[i]='\0';
    return final_str;
}

char *tohex(uint32_t num)
{
    char hexset[] = "0123456789ABCDEF";
    char resset[100];
    char *hex_str;
    int i=0;
    if(num==0)
    {
        resset[i] = '0';
        resset[i++] = '0';
        resset[i++] = '\0';
        char *final_str = resset;
        return final_str;
    }

    while (num>0)
    {
        int rem = num%16;
        resset[i++] = hexset[rem];
        num /= 16;
    }
    resset[i] = '\0';
    //reverse the string
    int s=0,e=i-1;
    while(s<e)
    {
        char temp = resset[e];
        resset[e]=resset[s];
        resset[s]=temp;
        s++;
        e--;
    }

    hex_str = resset;
    return hex_str;
}

char *percentage(char *cpu)
{
    char final[10];
    char *final_str;
    if (length_str(cpu) == 1)
    {
        final[0] = '0';
        final[1] = '.';
        final[2] = cpu[0];
        final[3] = '\0';
    }
    else if (length_str(cpu) == 2)
    {
        final[0] = '0';
        final[1] = '.';
        final[2] = cpu[0];
        final[3] = cpu[1];
        final[4] = '\0';
    }
    else
    {
        uint8_t idx = 0;
        uint8_t final_idx = 0;
        while (cpu[idx] != '\0')
        {
            if (idx == (length_str(cpu) - 2))
            {
                final[final_idx] = '.';
                final_idx++;
            }
            final[final_idx] = cpu[idx];
            final_idx++;
            idx++;
        }
        final[final_idx] = '\0';
    }
    final_str = final;
    return final_str;
}
