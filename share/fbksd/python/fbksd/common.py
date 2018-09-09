import os.path
import xml.etree.ElementTree as ET
import json
import glob
import re
import shutil
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


# Loads the scenes from the scenes_profile file
def load_scenes(scenes_profile):
    g_scenes = {}
    g_scenes_names = {}
    g_renderers = {}

    if not os.path.isfile(scenes_profile):
        return {}, {}, {}

    # load all available scenes
    tree = ET.parse(scenes_profile)
    root = tree.getroot()
    for renderer in root.findall('rendering_server'):
        r = Renderer()
        r.name = renderer.get('name')
        r.path = renderer.get('path')
        for scene in renderer.find('scenes').findall('scene'):
            s = Scene()
            s.renderer = r
            s.name = scene.get('name')
            s.path = scene.get('path')
            s.ground_truth = scene.get('ground_truth')

            if scene.find('dof_w') is not None:
                s.dof_w = float(scene.find('dof_w').get('value'))
            if scene.find('mb_w') is not None:
                s.mb_w = float(scene.find('mb_w').get('value'))
            if scene.find('ss_w') is not None:
                s.ss_w = float(scene.find('ss_w').get('value'))
            if scene.find('glossy_w') is not None:
                s.glossy_w = float(scene.find('glossy_w').get('value'))
            if scene.find('gi_w') is not None:
                s.gi_w = float(scene.find('gi_w').get('value'))

            spps = scene.find('spps').get('values').split()
            spps = [int(i) for i in spps]
            s.spps = spps

            regions = scene.find('regions')
            if regions:
                for region in regions.findall('region'):
                    regionEntry = ImageRegion()
                    regionEntry.xmin = int(region.find('xmin').get('value'))
                    regionEntry.ymin = int(region.find('ymin').get('value'))
                    regionEntry.xmax = int(region.find('xmax').get('value'))
                    regionEntry.ymax = int(region.find('ymax').get('value'))
                    regionEntry.dof_w = float(region.find('dof_w').get('value'))
                    regionEntry.mb_w = float(region.find('mb_w').get('value'))
                    regionEntry.ss_w = float(region.find('ss_w').get('value'))
                    regionEntry.glossy_w = float(region.find('glossy_w').get('value'))
                    regionEntry.gi_w = float(region.find('gi_w').get('value'))
                    s.regions.append(regionEntry)
            r.scenes.append(s)
            g_scenes[s.id] = s
            g_scenes_names[s.name] = s
        g_renderers[r.id] = r
    return g_scenes, g_scenes_names, g_renderers


def save_scenes_file(scenes_ids, g_scenes, current_slot_dir):
    data = {}
    #for key, scene in g_scenes.items():
    for key in scenes_ids:
        scene = g_scenes[key]
        scene_dict = {'id':scene.id, 'name':scene.name, 'renderer':scene.renderer.name, 'dof_w':scene.dof_w, 'mb_w':scene.mb_w,
                      'ss_w':scene.ss_w, 'glossy_w':scene.glossy_w, 'gi_w':scene.gi_w}
        scene_dict['reference'] = os.path.splitext(str(scene.ground_truth))[0] + '.png'
        scene_dict['thumbnail'] = os.path.splitext(str(scene.ground_truth))[0] + '_thumb256.jpg'
        spps = [{'spp_id':i, 'spp':spp} for i, spp in enumerate(scene.spps)]
        regions = [{'id':r.id, 'xmin':r.xmin, 'ymin':r.ymin, 'xmax':r.xmax, 'ymax':r.ymax,
                    'dof_w':r.dof_w, 'mb_w':r.mb_w, 'ss_w':r.ss_w, 'glossy_w':r.glossy_w, 'gi_w':r.gi_w} for r in scene.regions]
        scene_dict['spps'] = spps
        scene_dict['regions'] = regions
        # data.append(scene_dict)
        data[scene.id] = scene_dict
    with open(os.path.join(current_slot_dir, 'scenes.json'), 'w') as ofile:
        json.dump(data, ofile, indent=4)


def load_techniques(techniques_dir, techique_factory, technique_version_factory):
    # load all available denoisers
    # assumes folder name == filter name == executable name

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
            f.name = filter_name
            f.full_name = filter_info['full_name']
            f.comment = filter_info['comment']
            f.citation = filter_info['citation']
            if 'versions' in filter_info:
                for v in filter_info['versions']:
                    version = technique_version_factory()
                    version.technique = f
                    version.tag = v['name']
                    version.message = v['comment']
                    version.executable = v['executable']
                    if os.path.isfile(os.path.join(techniques_dir, filter_name, v['executable'])):
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
                version.executable = f.name
                if os.path.isfile(os.path.join(techniques_dir, filter_name, filter_name)):
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
    # load all available samplers
    # assumes folder name == sampler name == executable name
    def sample_factory():
        return Sampler()
    def sample_version_factory():
        return SamplerVersion()
    return load_techniques(samplers_dir, sample_factory, sample_version_factory)


def save_techniques_file(scenes_ids, versions_ids, techiques, filepath):
    data = []
    for key, f in techiques.items():
        filter_dict = {
            'id': f.id,
            'name': f.name,
            'full_name': f.full_name,
            'comment': f.comment,
            'citation': f.citation
        }
        versions = []
        for version in f.versions:
            if version.id not in versions_ids:
                continue

            version_dict = {
                'id': version.id,
                'tag': version.tag,
                'message': version.message,
                'status': version.status
            }

            if not version.results:
                continue
            results_ids = []
            for result in version.results:
                if result.scene.id in scenes_ids:
                    results_ids.append(result.id)
            version_dict['results_ids'] = results_ids
            versions.append(version_dict)

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
                # FIXME: this doesn't load results with spps not in the scene spp list
                for spp in scene.spps:
                    result_img_filename = os.path.join(version_results_dir, '{}_0.exr'.format(spp))
                    if not os.path.isfile(result_img_filename):
                        continue
                    errors_log_filename = os.path.join(version_results_dir, '{}_0_errors.json'.format(spp))
                    if not os.path.isfile(errors_log_filename):
                        continue
                    # skip old results
                    if os.path.getctime(result_img_filename) > os.path.getctime(errors_log_filename):
                        continue

                    result = result_factory()
                    result.filter_version = version
                    result.scene = scene
                    result.spp = spp
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


def save_techiques_result_file(filepath, scenes_ids, g_filters, versions_ids):
    data = {}
    filters = list(g_filters.values())
    for f in filters:
        for v in f.versions:
            if v.id not in versions_ids:
                continue

            for result in v.results:
                if result.scene.id not in scenes_ids:
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


def save_filters_result_file(current_slot_dir, scenes_ids, filters, versions_ids):
    save_techiques_result_file(os.path.join(current_slot_dir, 'results.json'), scenes_ids, filters, versions_ids)


def save_samplers_result_file(current_slot_dir, scenes_ids, samplers, versions_ids):
    save_techiques_result_file(os.path.join(current_slot_dir, 'samplers_results.json'), scenes_ids, samplers, versions_ids)


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
        if s.renderer.name in scenes_by_renderer:
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


def writeTempConfig(config_filename, current_config, renderers_dir, renderers_g, scenes_dir, scenes_names_g):
    # scenes = scenesFromIds(arg_scenes, g_scenes)
    # if not scenes:
    #     return [], ''
    # if arg_spps:
    #     arg_spps = list(set(arg_spps))
    #     arg_spps.sort()

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
            path = os.path.join(scenes_dir, scene_obj.path)
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


def currentConfigName(current_config_link):
    return os.path.splitext(os.readlink(current_config_link))[0]


def run_techniques(benchmark_exec, techniques_prefix, techiques, g_techiques_names, slot_prefix, config_filename, overwrite):
    # clear old shared memory files
    if os.path.isfile('/dev/shm/SAMPLES_MEMORY'):
        os.remove('/dev/shm/SAMPLES_MEMORY')
    if os.path.isfile('/dev/shm/RESULT_MEMORY'):
        os.remove('/dev/shm/RESULT_MEMORY')

    for f in techiques:
        filt = g_techiques_names[f['name']]
        for v in f['versions']:
            version = [ver for ver in filt.versions if ver.tag == v][0]
            print('Benchmarking: {}'.format(version.get_name()))
            out_folder = os.path.join(slot_prefix, filt.name, version.tag)
            filter_exec = os.path.join(techniques_prefix, filt.name, version.executable)
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
        m = re.match(r'(\w+)/(\w+)/(\w+)/(\d+)_.*errors\.json', error)
        if m:
            technique_name = m.group(1)
            version_name = m.group(2)
            scene_name = m.group(3)
            spp = m.group(4)

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


def compare_techniques_results(results_prefix, scenes_names, scenes_dir, exr2png_exec, compare_exec, overwrite = False):
    if not os.path.exists(results_prefix):
        return

    with cd(results_prefix):
        results = glob.glob('*/*/*/*.exr')
        errors_dict = make_error_logs_dict()

        for r in results:
            m = re.match(r'(\w+)/(\w+)/(\w+)/(\d+)_.*\.exr', r)
            technique_name = m.group(1)
            version_name = m.group(2)
            scene_name = m.group(3)
            spp = m.group(4)
            s = scenes_names[scene_name]

            img1 = os.path.join(scenes_dir, s.ground_truth)
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


def print_table(table_title, rows_title, cols_title, rows_labels, cols_labels, data, row_size=20, col_size=8):
    row_label_size = row_size
    col_label_size = col_size
    header_size = row_label_size + (col_label_size*len(cols_labels)) + len(cols_labels)

    # table title
    print('═'*header_size)
    print('{:^{header_size}s}'.format(table_title, header_size=header_size))
    print('═'*header_size)

    if cols_title != '' and rows_title != '':
        # rows title
        print('{0:^{col_width}s}│'.format(rows_title, col_width=row_label_size), end='')
        # cols title
        print('{0:^{col_width}s}│'.format(cols_title, col_width=len(cols_labels)*col_label_size + len(cols_labels) - 1))

    print(' '*row_label_size + '│' + ('{:^{col_size}}│'*len(cols_labels)).format(*cols_labels, col_size=col_label_size))
    # print('─'*header_size)
    for row_i, row in enumerate(data):
        print('{:<{row_label_size}.{row_label_size}s}│'.format(rows_labels[row_i], row_label_size=row_label_size), end='')
        for value in row:
            if value:
                print('{0:={col_size}.2f}│'.format(value, col_size=col_label_size), end='')
            else:
                print('{0:^{col_size}.2s}│'.format('', col_size=col_label_size), end='')
        print()
    print('═'*header_size)


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
