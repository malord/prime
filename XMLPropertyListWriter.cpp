// Copyright 2000-2021 Mark H. P. Lord

#include "XMLPropertyListWriter.h"
#include "ScopedPtr.h"
#include "StringUtils.h"
#include "TextEncoding.h"

namespace Prime {

namespace {
    const char plistDocType[] = "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
}

bool XMLPropertyListWriter::write(Stream* stream, Log* log, const Value& value, const Options& options, size_t bufferSize, void* buffer)
{
    XMLWriter writer(XMLWriter::Options(), stream, log, bufferSize, buffer);

    writer.writeProcessingInstruction("xml", Format("version=\"1.0\" encoding=\"%s\"", options.getEncoding().c_str()));
    writer.writeText("\n");

    writer.writeRaw(plistDocType);

    writer.startElement("plist");
    writer.writeAttribute("version", "1.0");

    if (!write(&writer, value)) {
        return false;
    }

    writer.endElement();
    writer.end();

    return !writer.getErrorFlag();
}

bool XMLPropertyListWriter::write(XMLWriter* writer, const Value& value)
{
    char buffer[512];

    switch (value.getType()) {
    case Value::TypeNull:
    case Value::TypeUndefined:
        break;

    case Value::TypeBool:
        if (value.getBool()) {
            writer->startElement("true");
        } else {
            writer->startElement("false");
        }
        writer->endElement();
        return !writer->getErrorFlag();

    case Value::TypeInteger:
        StringFormat(buffer, sizeof(buffer), "%" PRIME_PRId_VALUE, value.getInteger());
        writer->writeTextElement("integer", buffer);
        return !writer->getErrorFlag();

    case Value::TypeReal:
        StringFormat(buffer, sizeof(buffer), "%" PRIME_PRIg_VALUE, value.getReal());
        writer->writeTextElement("real", buffer);
        return !writer->getErrorFlag();

    case Value::TypeDate: {
        value.getDate().toISO8601(buffer, sizeof(buffer));
        writer->writeTextElement("string", buffer);
        return !writer->getErrorFlag();
    }

    case Value::TypeTime: {
        value.getTime().toISO8601(buffer, sizeof(buffer));
        writer->writeTextElement("string", buffer);
        return !writer->getErrorFlag();
    }

    case Value::TypeDateTime: {
        value.getDateTime().toISO8601(buffer, sizeof(buffer), "T", "Z");
        writer->writeTextElement("date", buffer);
        return !writer->getErrorFlag();
    }

    case Value::TypeData:
        return writeData(writer, value.getData());

    case Value::TypeString:
        writer->writeTextElement("string", value.getString());
        return !writer->getErrorFlag();

    case Value::TypeVector:
        return writeArray(writer, value.getVector());

    case Value::TypeDictionary:
        return writeDictionary(writer, value.getDictionary());

    case Value::TypeObject: {
        Value serialised = value.toValue();
        if (serialised.isUndefined() || serialised.isObject()) {
            writer->getLog()->error(PRIME_LOCALISE("Object cannot be written to XML property list since it cannot be converted to a value."));
            return false;
        }

        return write(writer, serialised);
    }
    }

    writer->getLog()->error(PRIME_LOCALISE("Can't write null or undefined values to an XML property list."));
    return false;
}

bool XMLPropertyListWriter::writeData(XMLWriter* writer, const Data& data)
{
    size_t maxBase64Size = Base64ComputeMaxEncodedSize(data.size(), 0, 0);

    ScopedArrayPtr<char> base64(new char[maxBase64Size + 2]);

    base64[0] = '\n';

    size_t encodedSize = data.empty() ? 0 : Base64Encode(base64.get() + 1, maxBase64Size, &data[0], data.size(), 0);
    PRIME_ASSERT(encodedSize <= maxBase64Size);

    base64[(size_t)(encodedSize + 1)] = '\n';

    writer->writeTextElement("data", StringView(base64.get(), encodedSize + 2)); // +2 due to the surrounding newlines
    return !writer->getErrorFlag();
}

bool XMLPropertyListWriter::writeArray(XMLWriter* writer, const Value::Vector& array)
{
    writer->startElement("array");

    size_t size = array.size();
    for (size_t i = 0; i != size; ++i) {
        write(writer, array[i]);
    }

    writer->endElement();
    return !writer->getErrorFlag();
}

bool XMLPropertyListWriter::writeDictionary(XMLWriter* writer, const Value::Dictionary& dictionary)
{
    writer->startElement("dict");

    size_t size = dictionary.size();
    for (size_t i = 0; i != size; ++i) {
        const Value::Dictionary::value_type& pair = dictionary.pair(i);

        writer->writeTextElement("key", pair.first);

        write(writer, pair.second);
    }

    writer->endElement();
    return !writer->getErrorFlag();
}
}
