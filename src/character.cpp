#include "character.h"

char_file_u4::char_file_u4()
{

}

void mob_data::setObject(obj_data *o)
{
    object = o;
}

obj_data* mob_data::getObject(void)
{
    return object;
}

bool mob_data::isObject(void)
{
    return object != nullptr;
}