#include <QCoreApplication>
#include <scpiparser.h>
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    SCPIParser parser;

    return a.exec();
}
