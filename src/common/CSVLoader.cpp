
#include "CSVLoader.h"




void CSVLoader::CreateColumns()
{
    CreateColumns(
        [](char c){return c==',';},
        [](char c){return c=='\n';}
    );
}

void CSVLoader::CreateRows()
{
    CreateRows(
        [](char c){return c==',';},
        [](char c){return c=='\n';}
    );
}