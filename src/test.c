#include <errno.h>
#include <string.h>


int main(int argc, char const *argv[])
{
    printf("%s\n", strerror(104));
    return 0;
}
