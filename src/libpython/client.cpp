/*
 * Copyright (c) 2019 Jonas Deyson
 *
 * This software is released under the MIT License.
 *
 * You should have received a copy of the MIT License
 * along with this program. If not, see <https://opensource.org/licenses/MIT>
 */

#include "fbksd/client/BenchmarkClient.h"
#include <boost/python.hpp>
#include <boost/python/numpy.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/make_shared.hpp>
#include <memory>
using namespace fbksd;
namespace bp = boost::python;
namespace np = bp::numpy;


bool SceneInfo_has(const SceneInfo& info, const std::string& key)
{
    return info.has<int64_t>(key) || info.has<float>(key) || info.has<bool>(key) || info.has<std::string>(key);
}

bp::object SceneInfo_get(const SceneInfo& info, const std::string& key)
{
    if(info.has<int64_t>(key))
        return bp::long_(info.get<int64_t>(key));
    if(info.has<float>(key))
        return bp::make_tuple(info.get<float>(key));
    if(info.has<bool>(key))
        return bp::make_tuple(info.get<bool>(key));
    if(info.has<std::string>(key))
        return bp::make_tuple(info.get<std::string>(key));
    throw std::runtime_error("key \"" + key + "\" does not exist");
}

std::shared_ptr<BenchmarkClient> BenchmarkClient_ctor()
{
    bp::object sys = bp::object(bp::handle<>(PyImport_ImportModule("sys")));
    bp::list list = bp::extract<bp::list>(sys.attr("argv"));
      // Convert Python argv into C-style argc/argv.
    int argc = bp::extract<int>(list.attr("__len__")());
    char** argv = new char*[argc];
    for (int arg = 0; arg < argc; ++arg)
        argv[arg] = strdup(bp::extract<const char*>(list[arg]));

    return std::make_shared<BenchmarkClient>(argc, argv);
}

void BenchmarkClient_evaluateSamples(BenchmarkClient& self, int spp, bp::object consumer)
{
    self.evaluateSamples(SPP(spp), [&](const BufferTile& tile) {
        auto data = *tile.begin();
        auto array = np::from_data(data, np::dtype::get_builtin<float>(),
                                   bp::make_tuple(tile.height(), tile.width(), tile.getSPP(), tile.getSampleSize()),
                                   bp::make_tuple(sizeof(float)*tile.width()*tile.getSPP()*tile.getSampleSize(),
                                                  sizeof(float)*tile.getSPP()*tile.getSampleSize(),
                                                  sizeof(float)*tile.getSampleSize(),
                                                  sizeof(float)),
                                   bp::object());
        consumer(array, bp::make_tuple(tile.beginX(), tile.beginY()));
    });
}

void BenchmarkClient_setSampleLayout(BenchmarkClient& self, bp::list list)
{
    SampleLayout layout;
    auto len = bp::len(list);
    for(long i = 0; i < len; ++i)
    {
        auto str_extract = bp::extract<bp::str>(list[i]);
        if(str_extract.check())
            layout(bp::extract<std::string>(str_extract()));
        else
        {
            auto tuple_extract = bp::extract<bp::tuple>(list[i]);
            if(tuple_extract.check())
            {
                auto tup = tuple_extract();
                layout(bp::extract<std::string>(tup[0]))[bp::extract<int>(tup[1])];
            }
        }
    }
    self.setSampleLayout(layout);
}

np::ndarray BenchmarkClient_getResultBuffer(BenchmarkClient& self)
{
    auto data = self.getResultBuffer();
    auto info = self.getSceneInfo();
    auto w = info.get<int64_t>("width");
    auto h = info.get<int64_t>("height");
    return np::from_data(data, np::dtype::get_builtin<float>(),
                         bp::make_tuple(h, w, 3),
                         bp::make_tuple(sizeof(float)*w*3, sizeof(float)*3, sizeof(float)),
                         bp::object());
}


BOOST_PYTHON_MODULE(client)
{
    using namespace boost::python;
    Py_Initialize();
    np::initialize();

    bp::docstring_options doc;
    doc.disable_cpp_signatures();

    class_<SceneInfo>("SceneInfo",
                      "Contains information about the scene being rendered.\n\n"
                      "The list of available information can be found in the "
                      "`C++ API reference <https://fbksd.github.io/fbksd/docs/latest/classfbksd_1_1SceneInfo.html#details>`__.\n")
        .def("has", &SceneInfo_has, "Returns `True` if the given key exists.",
             args("key"))
        .def("get", &SceneInfo_get,
             "Returns the value for a given key. Throws an error if the key does not exists.",
             args("key"))
    ;

    class_<BenchmarkClient, boost::noncopyable>("BenchmarkClient",
        "The BenchmarkClient class is used to communicate with the benchmark server")
        .def("__init__", make_constructor(BenchmarkClient_ctor))
        .def("get_scene_info", &BenchmarkClient::getSceneInfo, args(""),
             ":return: Returns information about the scene being rendered.\n"
             ":rtype: :class:`~SceneInfo`.")
        .def("set_sample_layout", &BenchmarkClient_setSampleLayout, args("self", "layout"),
             "Sets the sample layout.\n\n"
             "The layout is the way to inform the FBKSD server "
             "what data your technique requires for each sample, and how the data should "
             "be laid out in memory. "
             "Common data includes, for example, color RGB and (x,y) image plane position.\n\n"
             ":param list layout: List of sample components.\n"
             "    A component can be a string (Ex: ``'COLOR_R'``), or a pair with a string and a number indicating"
             "    the enumeration value (Ex: ``('NORMAL_X', 1)``, indication the x normal value for the second intersection point).\n"
             "    The list of available components can be found in the `C++ API reference <https://fbksd.github.io/fbksd/docs/latest/classfbksd_1_1SampleLayout.html#details>`__.\n")
        .def("evaluate_samples", &BenchmarkClient_evaluateSamples, args("self", "spp", "consumer"),
             "Request samples.\n\n"
             "The samples are computed by the renderer and sent in tiles. For each tile, the consumer callback\n"
             "function is called so you can consume the samples.\n\n"
             ":param spp: number of samples per pixel to evaluate.\n"
             ":param consumer: Consumer callback function.\n"
             "    The callback should accept two parameters: ``tile`` and ``offset``.\n\n"
             "    The ``tile`` is a `numpy.ndarray <https://docs.scipy.org/doc/numpy/reference/generated/numpy.ndarray.html#numpy.ndarray>`__ "
             "containing the tile data. It has 4 dimensions, in this order:\n"
             "    ``height``, ``width``, ``spp``, and ``sample_size``, where ``sample_size`` is the number of components\n"
             "    of the sample specified in the :func:`~set_sample_layout` method.\n\n"
             "    The ``offset`` is a tuple containing the :math:`(x, y)` pixel coordinates of the result image, corresponding to the"
             "    upper-left corner of the tile.\n\n"
             "    .. Note:: Your callback function should return as fast as possible."
             "       So, try to avoid raw loops when consuming the samples whenever possible.\n\n")
        .def("get_result_buffer", &BenchmarkClient_getResultBuffer, args("self"),
             "Returns a pointer to the buffer where the result image is stored.\n\n"
             "The user should write the final reconstructed image to this buffer before calling :func:`~send_result`.\n\n"
             ":return: A RGB image as a `numpy.ndarray <https://docs.scipy.org/doc/numpy/reference/generated/numpy.ndarray.html#numpy.ndarray>`__ "
             "with shape ``(height, width, 3)`` .\n")
        .def("send_result", &BenchmarkClient::sendResult, args("self"), "Sends the final result.")
    ;
}
