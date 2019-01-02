/*
 * Copyright (c) 2019 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

#include "fbksd/core/SampleLayout.h"


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

bool SampleLayout::hasInput(const std::string &name) const
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
