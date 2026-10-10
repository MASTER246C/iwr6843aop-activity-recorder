#include "serial_line.h"
int or_serial_feed(OrSerialLine *line,uint8_t byte)
{
    if(byte=='\n') {
        int valid=!line->discard;
        if(valid)line->text[line->used]='\0';
        line->used=0U;line->discard=0U;
        return valid?1:-1;
    }
    if(!line->discard) {
        if((byte<32U&&byte!='\r'&&byte!='\t')||byte>126U||line->used+1U>=sizeof(line->text))
            line->discard=1U;
        else line->text[line->used++]=(char)byte;
    }
    return 0;
}
