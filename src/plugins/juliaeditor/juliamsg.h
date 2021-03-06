#ifndef JULIAMSG_H
#define JULIAMSG_H

#include <QByteArray>
#include <QVector>
#include <QString>
#include <QDataStream>

// NOTE: These values MUST stay in sync with the julia side
// -----------------------------------------
char const * const NULL_name =         "null";
char const * const EVAL_name =         "eval";
char const * const EVAL_SILENT_name =  "eval-silent";
char const * const PACKAGE_name =      "package";
char const * const DIR_name =          "dir";
char const * const COMPLETE_name =     "complete";
char const * const WATCH_name =        "watch";

char const * const OUTPUT_ERROR_name =       "output-error";
char const * const OUTPUT_EVAL_name =        "output-eval";
char const * const OUTPUT_EVAL_SILENT_name = "output-eval-silent";
char const * const OUTPUT_PACKAGE_name =     "output-package";
char const * const OUTPUT_DIR_name =         "output-dir";
char const * const OUTPUT_PLOT_name =        "output-plot";
char const * const OUTPUT_COMPLETE_name =    "output-complete";
char const * const OUTPUT_WATCH_name =       "output-watch";
// -----------------------------------------

#if 0
class JuliaMsg
{
public:
  quint8 type;
  QVector<QString> params;


  void fromBytes( QByteArray& bytes )
  {
    QDataStream stream(&bytes, QIODevice::ReadOnly);
    stream.setVersion(QDataStream::Qt_4_0);
    stream.setByteOrder(QDataStream::LittleEndian);

    stream >> type;

    quint8 num_params;
    stream >> num_params;
    params.reserve(num_params);

    quint32 param_size;
    for (quint8 i = 0; i < num_params; ++i)
    {
      stream >> param_size;
      char* buff = new char[param_size + 1];
      buff[param_size] = 0;
      stream.readRawData(buff, param_size);
      params.push_back(QString::fromUtf8(buff, param_size));
      delete buff;
    }
  }

  void toBytes(QByteArray& bytes)
  {
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_4_0);
    stream.setByteOrder(QDataStream::LittleEndian);

    stream << type;
    stream << static_cast<qint8>(params.size());

    // when qt streams a char* it does so as the size and then the data
    for (int i = 0; i < params.size(); ++i)
      stream << params[i].toStdString().c_str();
  }
};
#endif



#endif // JULIAMSG_H
