#include "fbksd/core/SampleLayout.h"
#include <QDataStream>


SampleLayout &SampleLayout::operator()(const std::string &name, ElementIO io)
{
    ParameterEntry entry;
    entry.name = name;
    entry.io = io;
    parameters.push_back(entry);

    return *this;
}

SampleLayout &SampleLayout::operator[](int num)
{
    ParameterEntry& entry = parameters.back();
    entry.number = num;

    return *this;
}

SampleLayout &SampleLayout::setElementIO(const std::string &name, ElementIO io)
{
    for(std::size_t i = 0; i < parameters.size(); ++i)
        if(parameters[i].name == name)
            parameters[i].io = io;

    return *this;
}

SampleLayout &SampleLayout::setElementIO(int index, SampleLayout::ElementIO io)
{
    parameters[index].io = io;
    return *this;
}

int SampleLayout::getSampleSize() const
{
    return parameters.size();
}

int SampleLayout::getInputSize() const
{
    int result = 0;
    for(std::size_t i = 0; i < parameters.size(); ++i)
        if(parameters[i].io == INPUT)
            result += 1;
    return result;
}

int SampleLayout::getOutputSize() const
{
    return getSampleSize() - getInputSize();
}

bool SampleLayout::hasInput(const std::string &name)
{
    for(const auto& e: parameters)
    {
        if(e.name == name)
        {
            if(e.io == INPUT)
                return true;
            else
                return false;
        }
    }

    return false;
}

QDataStream& operator<<(QDataStream& stream, const SampleLayout& layout)
{
    stream << (quint64)layout.parameters.size();
    for(std::size_t i = 0; i < layout.parameters.size(); ++i)
    {
        const SampleLayout::ParameterEntry& par = layout.parameters[i];
        stream << QString::fromStdString(par.name);
        stream << par.number;
        stream << (bool)par.io;
    }

    return stream;
}

QDataStream& operator>>(QDataStream& stream, SampleLayout& layout)
{
    quint64 size = 0;
    stream >> size;

    layout.parameters.reserve(size);
    for(int i = 0; i < size; ++i)
    {
        SampleLayout::ParameterEntry par;
        QString name;
        stream >> name;
        par.name = name.toStdString();
        stream >> par.number;
        stream >> (bool&)par.io;
        layout.parameters.push_back(par);
    }

    return stream;
}

bool SampleLayout::isValid(const std::set<std::string> &reference) const
{
    std::set<std::string> counter;

    for(const auto& par: parameters)
    {
        // test invalid element
        if(reference.find(par.name) == reference.end())
            return false;

        // test repeated element
        // FIXME: this does not work because of enumerable elements
//        if(!counter.insert(par.name).second)
//            return false;
    }

    return true;
}
