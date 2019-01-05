# Copyright (c) 2019 Jonas Deyson
#
# This software is released under the MIT License.
#
# You should have received a copy of the MIT License
# along with this program. If not, see <https://opensource.org/licenses/MIT>

import os.path
import json
import glob
import re
import shutil
import itertools
import sys
from subprocess import call, Popen, PIPE, STDOUT
from contextlib import contextmanager
from fbksd.model import *


@contextmanager
def cd(newdir):
    prevdir = os.getcwd()
    os.chdir(os.path.expanduser(newdir))
    try:
        yield
    finally:
        os.chdir(prevdir)


def check_filters(path):
    if not os.path.isdir(path):
        print("ERROR: \'denoisers\' folder not found.")
        return False
    return True

def check_samplers(path):
    if not os.path.isdir(path):
        print("ERROR: \'samplers\' folder not found.")
        return False
    return True

def check_scenes(path):
    if not os.path.isdir(path):
        print("ERROR: \'scenes\' folder not found.")
        return False
    return True

def check_configs(path):
    if not os.path.isdir(path):
        print("ERROR: \'configs\' folder not found.")
        return False
    return True

def check_current_config(path):
    if not os.path.exists(path):
        print("ERROR: No current configuration.")
        return False
    return True

def check_results(path):
    if not os.path.isdir(path):
        print("ERROR: \'results\' folder not found.")
        return False
    return True

def check_current_slot(path):
    if not os.path.exists(path):
        print("ERROR: No current slot.")
        return False
    return True


def load_renderers(renderers_dir):
    renderers = {}
    with cd(renderers_dir):
        files = glob.glob('*/info.json')
        for file in files:
            with open(file) as f:
                data = json.load(f)
                r = Renderer()
                r.name = data['name']
                r.path = os.path.join(os.path.split(file)[0], data['exec'])
                r.is_ready = os.path.isfile(os.path.join(os.path.dirname(file), r.path))
                renderers[r.name] = r
    return renderers


# Loads the scenes from the scenes_profile file
def load_scenes(scenes_file, renderers_dir):
    g_scenes = {}
    g_scenes_names = {}

    if not os.path.exists(scenes_file):
        return {}, {}, {}

    scenes_cache = {}
    with open(scenes_file) as f:
        scenes_cache = json.load(f)

    g_renderers = load_renderers(renderers_dir)

    for renderer_item in scenes_cache:
        renderer_name = renderer_item['renderer']
        scenes = renderer_item['scenes']

        if renderer_name in g_renderers:
            renderer = g_renderers[renderer_name]
        else:
            renderer = Renderer()
            renderer.name = renderer_name
            g_renderers[renderer_name] = renderer

        for scene in scenes:
            s = Scene()
            s.renderer = renderer
            s.name = scene['name']
            s.path = scene['path']
            s.ground_truth = scene['ref-img']
            renderer.scenes.append(s)
            g_scenes[s.id] = s
            g_scenes_names[s.name] = s

    return g_scenes, g_scenes_names, g_renderers


def save_scenes_file(scenes, current_slot_dir):
    data = {}
    #for key, scene in g_scenes.items():
    for scene in scenes.values():
        if type(scene) == ConfigScene:
            scene = scene.scene
        scene_dict = {'id':scene.id, 'name':scene.name, 'renderer':scene.renderer.name, 'dof_w':scene.dof_w, 'mb_w':scene.mb_w,
                      'ss_w':scene.ss_w, 'glossy_w':scene.glossy_w, 'gi_w':scene.gi_w}
        scene_dict['reference'] = os.path.splitext(str(scene.get_reference()))[0] + '.png'
        scene_dict['thumbnail'] = os.path.splitext(str(scene.get_reference()))[0] + '_thumb256.jpg'
        regions = [{'id':r.id, 'xmin':r.xmin, 'ymin':r.ymin, 'xmax':r.xmax, 'ymax':r.ymax,
                    'dof_w':r.dof_w, 'mb_w':r.mb_w, 'ss_w':r.ss_w, 'glossy_w':r.glossy_w, 'gi_w':r.gi_w} for r in scene.regions]
        scene_dict['regions'] = regions
        # data.append(scene_dict)
        data[scene.id] = scene_dict
    with open(os.path.join(current_slot_dir, 'scenes.json'), 'w') as ofile:
        json.dump(data, ofile, indent=4)


def load_techniques(techniques_dir, techique_factory, technique_version_factory):
    g_filters = {}
    g_filters_names = {}
    g_filters_versions = {}

    if os.path.isdir(techniques_dir):
        filters_names = [name for name in os.listdir(techniques_dir)
                        if os.path.isdir(os.path.join(techniques_dir, name)) and name != 'SampleWriter']
        for filter_name in filters_names:
            filter_info = None
            info_filename = os.path.join(techniques_dir, filter_name, 'info.json')
            if not os.path.isfile(info_filename):
                print('Filter {} missing info.json file.'.format(filter_name))
                continue
            with open(os.path.join(techniques_dir, filter_name, 'info.json')) as info_file:
                filter_info = json.load(info_file)
            f = techique_factory()
            f.name = filter_info['short_name']
            f.full_name = filter_info['full_name']
            f.comment = filter_info['comment']
            f.citation = filter_info['citation']
            if 'versions' in filter_info:
                for v in filter_info['versions']:
                    version = technique_version_factory()
                    version.technique = f
                    version.tag = v['name']
                    version.message = v['comment']
                    version.executable = os.path.join(techniques_dir, filter_name, v['executable'])
                    if os.path.isfile(version.executable):
                        version.status = 'ready'
                    else:
                        version.status = 'not compiled'
                    f.versions.append(version)
                    g_filters_versions[version.id] = version
            else:
                version = technique_version_factory()
                version.technique = f
                version.tag = 'default'
                if 'comment' in filter_info and filter_info['comment'] != '':
                    version.message = filter_info['comment']
                else:
                    version.message = 'Default version'
                version.executable = os.path.join(techniques_dir, filter_name, f.name)
                if os.path.isfile(version.executable):
                    version.status = 'ready'
                else:
                    version.status = 'not compiled'
                f.versions.append(version)
                g_filters_versions[version.id] = version
            g_filters[f.id] = f
            g_filters_names[f.name] = f
    return g_filters, g_filters_names, g_filters_versions


def load_filters(filters_dir):
    def filter_factory():
        return Filter()
    def filter_version_factory():
        return FilterVersion()
    return load_techniques(filters_dir, filter_factory, filter_version_factory)


def load_samplers(samplers_dir):
    def sample_factory():
        return Sampler()
    def sample_version_factory():
        return SamplerVersion()
    return load_techniques(samplers_dir, sample_factory, sample_version_factory)


def save_techniques_file(scenes, versions_ids, techniques, filepath):
    data = []
    for key, f in techniques.items():
        filter_dict = {
            'id': f.id,
            'name': f.name,
            'full_name': f.full_name,
            'comment': f.comment,
            'citation': f.citation
        }
        versions = []
        for version in f.versions:
            if not version.results:
                continue
            if version.id not in versions_ids:
                continue

            results_ids = []
            for result in version.results:
                if result.scene.id in scenes:
                    s = scenes[result.scene.id]
                    if type(s) == ConfigScene and result.spp not in s.spps:
                        continue
                    results_ids.append(result.id)

            versions.append({
                'id': version.id,
                'tag': version.tag,
                'message': version.message,
                'status': version.status,
                'results_ids': results_ids
            })

        if not versions:
            continue
        filter_dict['versions'] = versions
        data.append(filter_dict)
    with open(filepath, 'w') as ofile:
        json.dump(data, ofile, indent=4)


def load_techniques_results(techniques, g_scenes, techniques_dir, result_factory):
    for tech in list(techniques.values()):
        filter_dir = os.path.join(techniques_dir, tech.name)
        if not os.path.isdir(filter_dir):
            continue
        for version in tech.versions:
            version_dir = os.path.join(filter_dir, version.tag)
            if not os.path.isdir(version_dir):
                continue
            for scene in list(g_scenes.values()):
                version_results_dir = os.path.join(version_dir, scene.name)
                if not os.path.isdir(version_results_dir):
                    continue

                spps = []
                errors_logs_by_spps = {}
                errors_logs = glob.glob(os.path.join(version_results_dir, '*_0_errors.json'))
                for errors_log_filename in errors_logs:
                    name = os.path.basename(errors_log_filename)
                    m = re.match(r'(\d+)_.*errors\.json', name)
                    if not m:
                        continue
                    spp = int(m.group(1))
                    spps.append(spp)
                    errors_logs_by_spps[spp] = errors_log_filename
                spps.sort()

                for spp in spps:
                    errors_log_filename = errors_logs_by_spps[spp]
                    result_img_filename = os.path.join(version_results_dir, '{}_0.exr'.format(spp))
                    if not os.path.isfile(result_img_filename):
                        continue
                    # skip old results
                    if os.path.getctime(result_img_filename) > os.path.getctime(errors_log_filename):
                        continue

                    result = result_factory()
                    result.filter_version = version
                    result.scene = scene
                    result.spp = int(spp)
                    main_log_filename = os.path.join(version_results_dir, '{}_0_log.json'.format(spp))
                    with open(main_log_filename) as log_file:
                        log_data = json.load(log_file)
                        result.aborted = log_data['aborted']
                        result.exec_time = log_data['reconstruction_time']['time_ms']
                        result.rendering_time = log_data['rendering_time']['time_ms']
                    with open(errors_log_filename) as log_file:
                        log_data = json.load(log_file)
                        result.mse = log_data['mse']
                        result.psnr = log_data['psnr']
                        result.ssim = log_data['ssim']
                        if 'rmse' in log_data:
                            result.rmse = log_data['rmse']
                    if result.aborted:
                        continue
                    # TODO: regions errors
                    version.results.append(result)


def load_filters_results(filters, scenes, current_slot_dir):
    def result_factory():
        return Result()
    load_techniques_results(filters, scenes, os.path.join(current_slot_dir, 'denoisers'), result_factory)


def load_samplers_results(samplers, scenes, current_slot_dir):
    def result_factory():
        return SamplerResult()
    load_techniques_results(samplers, scenes, os.path.join(current_slot_dir, 'samplers'), result_factory)


def save_techniques_result_file(filepath, scenes_by_id, g_filters, versions_ids):
    data = {}
    filters = list(g_filters.values())
    for f in filters:
        for v in f.versions:
            if v.id not in versions_ids:
                continue

            for result in v.results:
                if result.scene.id not in scenes_by_id:
                    continue
                scene = scenes_by_id[result.scene.id]
                if type(scene) == ConfigScene:
                    if result.spp not in scene.spps:
                        continue

                data[result.id] = {
                    'scene_id': result.scene.id,
                    'spp': result.spp,
                    'filter_version_id': result.filter_version.id,
                    'exec_time': result.exec_time,
                    'rendering_time': result.rendering_time,
                    'mse': result.mse,
                    'psnr': result.psnr,
                    'ssim': result.ssim,
                    'rmse': result.rmse,
                    'aborted': result.aborted
                    #TODO: regions
                }
    with open(filepath, 'w') as ofile:
        json.dump(data, ofile, indent=4)


def save_filters_result_file(current_slot_dir, scenes_by_id, filters, versions_ids):
    save_techniques_result_file(os.path.join(current_slot_dir, 'results.json'), scenes_by_id, filters, versions_ids)


def save_samplers_result_file(current_slot_dir, scenes_by_id, samplers, versions_ids):
    save_techniques_result_file(os.path.join(current_slot_dir, 'samplers_results.json'), scenes_by_id, samplers, versions_ids)


# get a list of ids and load the corresponding scenes
# if the list is empty, all scenes are loaded
def scenesFromIds(arg_scenes, g_scenes):
    scenes = []
    if arg_scenes:
        for sid in arg_scenes:
            if sid in g_scenes:
                scenes.append(g_scenes[sid])
            else:
                print('Error: no scene with ID = {} found!'.format(sid))
    else:
        scenes = list(g_scenes.values())
    return scenes


# get a list of ids and load the corresponding filters
# if the list is empty, all filters are loaded
# if arg_filters is None, no filter is loaded
def techniqueVersionsFromIds(ids, versions):
    if ids is None:
        return []

    techniques = []
    for fid in ids:
        if fid in versions:
            techniques.append(versions[fid])
        else:
            print('Error: no filter with ID = {} found!'.format(fid))
    return techniques


def get_configs(configs_dir, current_config_link):
    configs = []
    for config_file in os.listdir(configs_dir):
        path = os.path.join(configs_dir, config_file)
        if os.path.isfile(path):
            name,ext = os.path.splitext(config_file)
            if not os.path.islink(path):
                configs.append(
                    {'name': name,
                     'ctime': os.path.getctime(path)}
                )
    configs.sort(key=lambda x: x['ctime'], reverse=False)
    current_config = None
    if os.path.exists(current_config_link):
        current_config, ext = os.path.splitext(os.readlink(current_config_link))
    return configs, current_config


def new_config(scenes, filters, samplers, spps):
    scenes_by_renderer = {}
    for s in scenes:
        if s.renderer.name not in scenes_by_renderer:
            scenes_by_renderer[s.renderer.name] = [s]
        else:
            scenes_by_renderer[s.renderer.name].append(s)

    root_node = {}
    renderers_array = []
    for renderer in scenes_by_renderer:
        renderer_node = {}
        renderer_node['name'] = renderer
        scenes_array = []
        for scene in scenes_by_renderer[renderer]:
            scene_node = {}
            scene_node['name'] = scene.name
            scene_node['spps'] = spps
            scenes_array.append(scene_node)
        renderer_node['scenes'] = scenes_array
        renderers_array.append(renderer_node)
    root_node['renderers'] = renderers_array

    filters_dict = {}
    for fv in filters:
        if fv.technique.name not in filters_dict:
            filters_dict[fv.technique.name] = [fv.tag]
        else:
            filters_dict[fv.technique.name].append(fv.tag)
    filters_array = []
    for key, vers in filters_dict.items():
        f_node = {}
        f_node['name'] = key
        f_node['versions'] = vers
        filters_array.append(f_node)
    root_node['filters'] = filters_array

    samplers_dict = {}
    for sv in samplers:
        if sv.technique.name not in samplers_dict:
            samplers_dict[sv.technique.name] = [sv.tag]
        else:
            samplers_dict[sv.technique.name].append(sv.tag)
    samplers_array = []
    for key, vers in samplers_dict.items():
        s_node = {}
        s_node['name'] = key
        s_node['versions'] = vers
        samplers_array.append(s_node)
    root_node['samplers'] = samplers_array
    return root_node


def write_tmp_config(config_filename, current_config, renderers_dir, renderers_g, scenes_dir, scenes_names_g):
    with open(current_config) as f:
        config = json.load(f)

    if not config['renderers']:
        return None
    if not config['filters'] and not config['samplers']:
        return None

    for renderer in config['renderers']:
        renderer_obj = [r for r in renderers_g.values() if r.name == renderer['name']][0]
        path = os.path.join(renderers_dir, renderer_obj.path)
        renderer['path'] = os.path.abspath(path)
        for scene in renderer['scenes']:
            scene_obj = scenes_names_g[scene['name']]
            path = os.path.join(scenes_dir, renderer_obj.name, scene_obj.path)
            scene['path'] = os.path.abspath(path)
            # scene_info_node = {}
            # scene_info_node['has_dof'] = scene.dof_w > 0
            # scene_info_node['has_motion_blur'] = scene.mb_w > 0
            # scene_info_node['has_area_light'] = scene.ss_w > 0
            # scene_info_node['has_glossy_materials'] = scene.glossy_w > 0
            # scene_info_node['has_gi'] = scene.gi_w > 0
            # scene_node['scene_info'] = scene_info_node

    with open(config_filename, 'w') as outfile:
        json.dump(config, outfile, indent=4)
    return config


def current_config_name(current_config_link):
    return os.path.splitext(os.readlink(current_config_link))[0]


def run_techniques(benchmark_exec, techniques_prefix, techiques, g_techiques_names, slot_prefix, config_filename, overwrite):
    # clear old shared memory files
    if os.path.isfile('/dev/shm/SAMPLES_MEMORY'):
        os.remove('/dev/shm/SAMPLES_MEMORY')
    if os.path.isfile('/dev/shm/RESULT_MEMORY'):
        os.remove('/dev/shm/RESULT_MEMORY')

    for f in techiques:
        filt = g_techiques_names[f['name']]
        if 'versions' in f:
            versions = f['versions']
        else:
            versions = ['default']
        for v in versions:
            version = [ver for ver in filt.versions if ver.tag == v][0]
            print('Benchmarking: {}'.format(version.get_name()))
            out_folder = os.path.join(slot_prefix, filt.name, version.tag)
            filter_exec = version.executable
            benchmark_args = [benchmark_exec, '--config', config_filename, '--filter', filter_exec, '--output', out_folder]
            if not overwrite:
                benchmark_args.append('--resume')
            p = Popen(benchmark_args, stdout=PIPE, stderr=STDOUT, universal_newlines=True)

            while p.poll() is None:
                l = p.stdout.readline()
                print(l, end='')
            print(p.stdout.read())


def make_error_logs_dict():
    errors_dict = {}
    errors = glob.glob('*/*/*/*_errors.json')
    for error in errors:
        m = re.match(r'([a-zA-Z0-9_ ]+)/([a-zA-Z0-9_ ]+)/([a-zA-Z0-9_ ]+)/(\d+)_.*errors\.json', error)
        if m:
            technique_name = m.group(1)
            version_name = m.group(2)
            scene_name = m.group(3)
            spp = int(m.group(4))

            if technique_name in errors_dict:
                s1 = errors_dict[technique_name]
                if version_name in s1:
                    s2 = s1[version_name]
                    if scene_name in s2:
                        s3 = s2[scene_name]
                        s3[spp] = error
                    else:
                        s2[scene_name] = {spp: error}
                else:
                    s1[version_name] = {scene_name: {spp: error}}
            else:
                errors_dict[technique_name] = {version_name: {scene_name: {spp: error}}}
    return errors_dict


def get_error_log(errors_dict, technique_name, version_name, scene_name, spp):
    log = None
    if technique_name in errors_dict:
        d1 = errors_dict[technique_name]
        if version_name in d1:
            d2 = d1[version_name]
            if scene_name in d2:
                d3 = d2[scene_name]
                if spp in d3:
                    log = d3[spp]
    return log


def compare_techniques_results(results_prefix, scenes_names, versions, scenes_dir, exr2png_exec, compare_exec, overwrite = False):
    if not os.path.exists(results_prefix):
        return

    with cd(results_prefix):
        results = glob.glob('*/*/*/*.exr')
        errors_dict = make_error_logs_dict()

        for r in results:
            m = re.match(r'([a-zA-Z0-9_ ]+)/([a-zA-Z0-9_ ]+)/([a-zA-Z0-9_ ]+)/(\d+)_.*\.exr', r)
            if not m:
                continue
            technique_name = m.group(1)
            version_name = m.group(2)
            scene_name = m.group(3)
            spp = int(m.group(4))
            if scene_name not in scenes_names:
                continue
            if versions and ('{}-{}'.format(technique_name, version_name) not in versions):
                continue
            s = scenes_names[scene_name]
            if type(s) == ConfigScene:
                if spp not in s.spps:
                    continue
                s = s.scene

            img1 = os.path.join(scenes_dir, s.renderer.name, s.ground_truth)
            if not os.path.isfile(img1):
                print('Error: ground truth {} for scene {} not found.'.format(img1, s.name))
                continue

            img2 = os.path.join(results_prefix, r)
            if not os.path.isfile(img2):
                continue

            # skip results already computed
            if not overwrite:
                log = get_error_log(errors_dict, technique_name, version_name, scene_name, spp)
                if log:
                    log_ctime = os.path.getctime(log)
                    if log_ctime > os.path.getctime(img2) and log_ctime > os.path.getctime(img1):
                        continue

            call([exr2png_exec, img2])

            # compare the full result with the ground truth end parse errors
            p = Popen([compare_exec, '--save-maps', '--save-errors', img1, img2], stdout=PIPE, stdin=PIPE, stderr=PIPE)
            p.wait()
            # copy maps and errors log to results folder
            dest_prefix = os.path.join(results_prefix, technique_name, version_name, scene_name)
            shutil.move('errors.json', os.path.join(dest_prefix, '{}_0_errors.json'.format(spp)))
            shutil.move('mse_map.png', os.path.join(dest_prefix, '{}_0_mse_map.png'.format(spp)))
            shutil.move('rmse_map.png', os.path.join(dest_prefix, '{}_0_rmse_map.png'.format(spp)))
            shutil.move('ssim_map.png', os.path.join(dest_prefix, '{}_0_ssim_map.png'.format(spp)))
            print('{}, {}, {} spp'.format(technique_name, scene_name, spp))


def print_table_simple(table_title, cols_labels, data):
    lengths = [len(l) for l in cols_labels]
    for row in data:
        for i, l in enumerate(lengths):
            lengths[i] = max(l, len(row[i]))

    total_length = sum(lengths)
    header_size = total_length + len(cols_labels) + 1
    print('┌' + '─'*header_size + '┐')
    print('│{:^{header_size}}│'.format(table_title, header_size=header_size))
    print('├' + '─'*header_size + '┤')
    print('│', end='')
    for i, l in enumerate(cols_labels):
        print('{: ^{col_size}}│'.format(l, col_size=lengths[i] + 1), end='')
    print('\n├' + '─'*header_size + '┤')
    for row in data:
        print('│', end='')
        for i, value in enumerate(row):
            print('{0:{col_size}}│'.format(value, col_size=lengths[i] + 1), end='')
        print()
    print('└' + '─'*header_size + '┘')


def print_table(table_title, rows_title, cols_title, rows_labels, cols_labels, data, row_size=20, col_size=8):
    row_label_size = row_size + 2
    col_label_size = col_size + 2
    header_size = row_label_size + (col_label_size*len(cols_labels)) + len(cols_labels)

    # table title
    print('┌' + '─'*header_size + '┐')
    print('│{:^{header_size}s}│'.format(table_title, header_size=header_size))
    print('├' + '─'*header_size + '┤')

    if cols_title != '' and rows_title != '':
        # rows title
        print('│{0:^{col_width}s}│'.format(rows_title, col_width=row_label_size), end='')
        # cols title
        print('{0:^{col_width}s}│'.format(cols_title, col_width=len(cols_labels)*col_label_size + len(cols_labels) - 1))

    print('│' + ' '*row_label_size + '│' + ('{:^{col_size}}│'*len(cols_labels)).format(*cols_labels, col_size=col_label_size))
    # print('─'*header_size)
    for row_i, row in enumerate(data):
        print('│{:<{row_label_size}.{row_label_size}s}│'.format(rows_labels[row_i], row_label_size=row_label_size), end='')
        for value in row:
            if value:
                print('{0:={col_size}.6f}│'.format(value, col_size=col_label_size), end='')
            else:
                print('{0:^{col_size}.6s}│'.format('', col_size=col_label_size), end='')
        print()
    print('└' + '─'*header_size + '┘')


def print_results(versions, scenes, metrics):
    no_results = True
    names = [f.get_name() for f in versions]
    for scene in scenes:
        if type(scene) == Scene:
            spps = set()
            for v in versions:
                results = v.get_results(scene)
                v_spps = {r.spp for r in results}
                spps |= v_spps
        elif type(scene) == ConfigScene:
            spps = scene.spps
            scene = scene.scene

        for metric in metrics:
            data = [[None for y in spps] for x in versions]
            spps_names = [str(spp) for spp in spps]
            table_has_results = False
            for row_i, v in enumerate(versions):
                for col_i, spp in enumerate(spps):
                    result = v.get_result(scene, spp)
                    if result:
                        data[row_i][col_i] = getattr(result, metric)
                        table_has_results = True
            if table_has_results:
                print_table(scene.get_name(), ' ', metric, names, spps_names, data)
                no_results = False

    if no_results:
        print('No results.')


def print_csv(scenes, versions, metrics, mse_scale, rmse_scale):
    def print_table_csv(data):
        for row in data:
            for value in row:
                if value:
                    print('{},'.format(value), end='')
                else:
                    print(',', end='')
            print()

    def print_errors(scenes, versions, metrics):
        data = []
        data.append([None for y in itertools.product(versions, metrics)])
        data.append([None for y in itertools.product(versions, metrics)])
        data[0].extend([None, None])
        data[1].extend([None, None])
        for i, v in enumerate(versions):
            data[0][(i * len(metrics)) + 2] = v.get_name()
        for i, p in enumerate(itertools.product(versions, metrics)):
            data[1][i + 2] = p[1]

        row_i = 0
        for scene in scenes:
            spps = set()
            for v in versions:
                results = v.get_results(scene)
                v_spps = {r.spp for r in results}
                spps |= v_spps
            spps = list(spps)
            spps.sort()

            for i, spp in enumerate(spps):
                data.append([None for y in itertools.product(versions, metrics)])
                data[-1].extend([None, None])
                if i == 0:
                    data[row_i + 2][0] = scene.get_name()
                data[row_i + 2][1] = spp
                col_i = 0
                for f in versions:
                    result = f.get_result(scene, spp)
                    for metric in metrics:
                        if result:
                            value = 0.0
                            if metric == 'mse':
                                value = getattr(result, metric) * mse_scale
                            elif metric == 'rmse':
                                value = getattr(result, metric) * rmse_scale
                            else:
                                value = getattr(result, metric)
                            data[row_i + 2][col_i + 2] = '{:.4f}'.format(value)
                        col_i += 1
                row_i += 1
        print_table_csv(data)
    print_errors(scenes, versions, metrics)


def get_slots(results_dir):
    slots = []
    current_slot = ''
    for name in os.listdir(results_dir):
        path = os.path.join(results_dir, name)
        if os.path.isdir(path):
            if os.path.islink(path):
                current_slot = os.path.basename(os.readlink(path))
            else:
                slots.append(
                    {'name': name,
                     'ctime': os.path.getctime(path)}
                )
    slots.sort(key=lambda x: x['ctime'], reverse=False)
    return slots, current_slot


def scan_scenes(scenes_dir):
    """Scan scenes folder for fbksd-scene.json and fbksd-scenes.json files and builds a .fbksd-scenes-cache.json cache file."""

    if not os.path.exists(scenes_dir):
        print('ERROR: scenes folder doesn\'t exist.')
        return

    def append_scene(scene, scenes):
        name = scene['name']
        path = scene['path']
        ref_img = scene['ref-img']
        ref = ''
        if 'ref' in scene:
            ref = scene['ref']
        scenes.append({'name': name, 'path':path, 'ref-img':ref_img, 'ref':ref})

    data = []
    with os.scandir(scenes_dir) as it:
        for entry in it:
            if entry.name.startswith('.') or not entry.is_dir():
                continue
            renderer = entry.name
            with cd(os.path.join(scenes_dir, renderer)):
                scene_files = glob.glob('**/fbksd-scene.json', recursive=True)
                scenes = []
                for sf in scene_files:
                    with open(sf) as f:
                        scene = json.load(f)
                    scene['path'] = os.path.join(os.path.split(sf)[0], scene['path'])
                    scene['ref-img'] = os.path.join(os.path.split(sf)[0], scene['ref-img'])
                    append_scene(scene, scenes)

                scenes_files = glob.glob('**/fbksd-scenes.json', recursive=True)
                for sf in scenes_files:
                    scenes_vec = []
                    with open(sf) as f:
                        scenes_vec = json.load(f)
                    for scene in scenes_vec:
                        scene['path'] = os.path.join(os.path.split(sf)[0], scene['path'])
                        scene['ref-img'] = os.path.join(os.path.split(sf)[0], scene['ref-img'])
                        append_scene(scene, scenes)

                if scenes:
                    data.append({'renderer': renderer, 'scenes': scenes})

    with open(os.path.join(scenes_dir, '.fbksd-scenes-cache.json'), 'w') as ofile:
        json.dump(data, ofile, indent=4)


def load_saved_results(results_dir):
    with open(os.path.join(results_dir, 'scenes.json')) as f:
        scenes_dic = json.load(f)
    with open(os.path.join(results_dir, 'filters.json')) as f:
        filters_dic = json.load(f)
    with open(os.path.join(results_dir, 'samplers.json')) as f:
        samplers_dic = json.load(f)
    with open(os.path.join(results_dir, 'results.json')) as f:
        filters_results_dic = json.load(f)
    with open(os.path.join(results_dir, 'samplers_results.json')) as f:
        samplers_results_dic = json.load(f)

    renderers = {}
    scenes = {}
    for s in scenes_dic.values():
        scene = Scene()
        scene.id = s['id']
        scene.name = s['name']
        if s['renderer'] in renderers:
            scene.renderer = renderers[s['renderer']]
        else:
            r = Renderer()
            r.name = s['renderer']
            renderers[s['renderer']] = r
            scene.renderer = r
        scenes[scene.id] = scene

    filters = []
    for f in filters_dic:
        filt = Filter()
        filt.id = f['id']
        filt.name = f['name']
        for v in f['versions']:
            version = FilterVersion()
            version.technique = filt
            version.id = v['id']
            version.tag = v['tag']
            for rid in v['results_ids']:
                r = filters_results_dic[str(rid)]
                result = Result()
                result.scene = scenes[r['scene_id']]
                result.filter_version = version
                result.spp = r['spp']
                if result.spp not in result.scene.spps:
                    result.scene.spps.append(result.spp)
                result.exec_time = r['exec_time']
                result.rendering_time = r['rendering_time']
                result.mse = r['mse']
                result.psnr = r['psnr']
                result.ssim = r['ssim']
                result.rmse = r['rmse']
                result.aborte = r['aborted']
                version.results.append(result)
            filt.versions.append(version)
            filters.append(version)

    samplers = []
    for f in samplers_dic:
        filt = Sampler()
        filt.id = f['id']
        filt.name = f['name']
        for v in f['versions']:
            version = SamplerVersion()
            version.technique = filt
            version.id = v['id']
            version.tag = v['tag']
            for rid in v['results_ids']:
                r = samplers_results_dic[rid]
                result = SamplerResult()
                result.sampler_version = version
                result.scene = scenes[r['scene_id']]
                result.spp = r['spp']
                if result.spp not in result.scene.spps:
                    result.scene.spps.append(result.spp)
                result.exec_time = r['exec_time']
                result.rendering_time = r['rendering_time']
                result.mse = r['mse']
                result.psnr = r['psnr']
                result.ssim = r['ssim']
                result.rmse = r['rmse']
                result.aborte = r['aborted']
                version.results.append(result)
            filt.versions.append(version)
            samplers.append(filt)

    for s in scenes.values():
        s.spps.sort()

    return scenes, filters, samplers


def query_yes_no(question, default="yes"):
    """Ask a yes/no question via input() and return their answer.

    "question" is a string that is presented to the user.
    "default" is the presumed answer if the user just hits <Enter>.
        It must be "yes" (the default), "no" or None (meaning
        an answer is required of the user).

    The "answer" return value is True for "yes" or False for "no".
    """
    valid = {"yes": True, "y": True, "ye": True,
             "no": False, "n": False}
    if default is None:
        prompt = " [y/n] "
    elif default == "yes":
        prompt = " [Y/n] "
    elif default == "no":
        prompt = " [y/N] "
    else:
        raise ValueError("invalid default answer: '%s'" % default)

    while True:
        sys.stdout.write(question + prompt)
        choice = input().lower()
        if default is not None and choice == '':
            return valid[default]
        elif choice in valid:
            return valid[choice]
        else:
            sys.stdout.write("Please respond with 'yes' or 'no' "
                             "(or 'y' or 'n').\n")


def rank_techniques(title, scenes, versions, metrics):
    all_spps = set()
    for f in versions:
        for r in f.results:
            all_spps.add(r.spp)
    spps = all_spps
    for spp in all_spps:
        for f in versions:
            spps = spps & {r.spp for r in f.results}
    spps = sorted(spps)

    ranks = {f:0.0 for f in versions}
    num_ranks = 0
    for scene in scenes:
        for spp in spps:
            for metric in metrics:
                num_ranks = num_ranks + 1
                errors = []
                for f in versions:
                    r = f.get_result(scene, spp)
                    if not r:
                        continue
                    v = getattr(r, metric.name)
                    errors.append((f, v))
                errors = sorted(errors, key=lambda p: p[1], reverse=not metric.lower_is_better)
                for i, p in enumerate(errors):
                    ranks[p[0]] = ranks[p[0]] + i + 1
    if num_ranks == 0:
        return
    ranks = {f:(r/num_ranks) for f,r in ranks.items()}
    table = [[v, f.get_name()] for f, v in ranks.items()]
    table = sorted(table, key=lambda p: p[0])
    table = [[str(r[0]), r[1]] for r in table]
    print_table_simple(title, ['#', 'Technique'], table)


class Config:
    config_file = ''
    scenes = []
    scenes_names = set()
    filter_versions = []
    sampler_versions = []

    def __init__(self, config_file, all_scenes, all_filters, all_samplers):
        with open(config_file) as f:
            self.config_file = config_file
            config = json.load(f)
            for r in config['renderers']:
                for s in r['scenes']:
                    name = s['name']
                    scene = all_scenes.get(name)
                    if not scene:
                        continue
                    spps = s['spps']
                    spps = list(set(spps))
                    spps.sort()
                    self.scenes.append(ConfigScene(scene, spps))
                    self.scenes_names.add(name)

            for f in config['filters']:
                name = f['name']
                filt = all_filters.get(name)
                if not filt:
                    continue
                if 'versions' in f:
                    versions = f['versions']
                else:
                    versions = ['default']
                for v in versions:
                    version = filt.get_version(v)
                    if not version:
                        continue
                    self.filter_versions.append(version)

            for s in config['samplers']:
                name = s['name']
                sampler = all_samplers.get(name)
                if not sampler:
                    continue
                if 'versions' in s:
                    versions = s['versions']
                else:
                    versions = ['default']
                for v in versions:
                    version = sampler.get_version(v)
                    if not version:
                        continue
                    self.sampler_versions.append(version)

    def add_scene(self, scene, spps):
        if scene.name not in self.scenes_names:
            self.scenes.append(ConfigScene(scene, spps))
            self.scenes_names.add(scene.name)

    def remove_scene(self, scene):
        if scene.name in self.scenes_names:
            for i, s in enumerate(self.scenes):
                if s.get_name() == scene.name:
                    del self.scenes[i]
                    self.scenes_names.remove(scene.name)
                    return

    def add_filter(self, version):
        if version not in self.filter_versions:
            self.filter_versions.append(version)

    def remove_filter(self, version):
        if version in self.filter_versions:
            self.filter_versions.remove(version)

    def add_sampler(self, version):
        if version not in self.sampler_versions:
            self.sampler_versions.append(version)

    def remove_sampler(self, version):
        if version in self.sampler_versions:
            self.sampler_versions.remove(version)

    def print(self):
        if self.scenes:
            print('Scenes')
            print('{0:<5s}{1:<40s}{2:<10s}'.format('Id', 'Scene', 'spps'))
            print('{0:75s}'.format('-'*75))
            for scene in self.scenes:
                print('{0:<5s}{1:<40s}{2:<10s}'.format(str(scene.scene.id), scene.get_name(), str(scene.spps)))
            print('{0:75s}'.format('-'*75))

        if self.filter_versions:
            print('\nFilters')
            print('{0:<5s}{1:<20s}{2:<10s}'.format('Id', 'Filter', 'Status'))
            print('{0:75s}'.format('-'*75))
            for v in self.filter_versions:
                if v.tag == 'default':
                    print('{0:<5s}{1:<20s}{2:<10s}'.format(str(v.id), v.technique.name, v.status))
                else:
                    print('{0:<5s}{1:<20s}{2:<10s}'.format(str(v.id), v.technique.name + '-{}'.format(v.tag), v.status))
            print('{0:75s}'.format('-'*75))

        if self.sampler_versions:
            print('\nSamplers')
            print('{0:<5s}{1:<20s}{2:<10s}'.format('Id', 'Sampler', 'Status'))
            print('{0:75s}'.format('-'*75))
            for v in self.sampler_versions:
                if v.tag == 'default':
                    print('{0:<5s}{1:<20s}{2:<10s}'.format(str(v.id), v.technique.name, v.status))
                else:
                    print('{0:<5s}{1:<20s}{2:<10s}'.format(str(v.id), v.technique.name + '-{}'.format(v.tag), v.status))
            print('{0:75s}'.format('-'*75))

    def save(self):
        scenes_by_renderer = {}
        for s in self.scenes:
            if s.scene.renderer.name not in scenes_by_renderer:
                scenes_by_renderer[s.scene.renderer.name] = [s]
            else:
                scenes_by_renderer[s.scene.renderer.name].append(s)

        root_node = {}
        renderers_array = []
        for renderer in scenes_by_renderer:
            renderer_node = {}
            renderer_node['name'] = renderer
            scenes_array = []
            for scene in scenes_by_renderer[renderer]:
                scene_node = {}
                scene_node['name'] = scene.get_name()
                scene_node['spps'] = scene.spps
                scenes_array.append(scene_node)
            renderer_node['scenes'] = scenes_array
            renderers_array.append(renderer_node)
        root_node['renderers'] = renderers_array

        filters_dict = {}
        for fv in self.filter_versions:
            if fv.technique.name not in filters_dict:
                filters_dict[fv.technique.name] = [fv.tag]
            else:
                filters_dict[fv.technique.name].append(fv.tag)
        filters_array = []
        for key, vers in filters_dict.items():
            f_node = {}
            f_node['name'] = key
            f_node['versions'] = vers
            filters_array.append(f_node)
        root_node['filters'] = filters_array

        samplers_dict = {}
        for sv in self.sampler_versions:
            if sv.technique.name not in samplers_dict:
                samplers_dict[sv.technique.name] = [sv.tag]
            else:
                samplers_dict[sv.technique.name].append(sv.tag)
        samplers_array = []
        for key, vers in samplers_dict.items():
            s_node = {}
            s_node['name'] = key
            s_node['versions'] = vers
            samplers_array.append(s_node)
        root_node['samplers'] = samplers_array
        with open(self.config_file, 'w') as outfile:
            json.dump(root_node, outfile, indent=4)
