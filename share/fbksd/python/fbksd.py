#!/usr/bin/env python3

# Copyright (c) 2019 Jonas Deyson
#
# This software is released under the MIT License.
#
# You should have received a copy of the MIT License
# along with this program. If not, see <https://opensource.org/licenses/MIT>

import argparse
from argparse import RawTextHelpFormatter
import os.path
import sys
import shutil
import subprocess
import json
import itertools
import time
import http.server
import socketserver
# local imports
from fbksd.common import *


#=============================================
#                Common paths                #
#=============================================
scenes_dir         = os.path.join(os.getcwd(), 'scenes')
renderers_dir      = os.path.join(os.getcwd(), 'renderers')
denoisers_dir      = os.path.join(os.getcwd(), 'denoisers')
samplers_dir       = os.path.join(os.getcwd(), 'samplers')
configs_dir        = os.path.join(os.getcwd(), 'configs')
current_config     = os.path.join(configs_dir, '.current.json')
page_dir           = os.path.join(os.getcwd(), '.page')
results_dir        = os.path.join(os.getcwd(), 'results')
current_slot_dir   = os.path.join(results_dir, '.current')
scenes_file        = os.path.join(scenes_dir, '.fbksd-scenes-cache.json')
install_prefix_dir = os.getenv('FBKSD_INSTALL_DIR',
    os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), os.pardir)))


#=============================================
#              Global variables              #
#=============================================
g_filters = {}
g_filters_names = {}
g_filters_versions = {}
g_samplers = {}
g_samplers_names = {}
g_samplers_versions = {}
g_renderers = {}
g_scenes = {}
g_scenes_names = {}
g_persistent_state = None
g_scenes_loaded = False
g_filters_loaded = False
g_samplers_loaded = False
g_results_loaded = False
g_samplers_results_loaded = False


def load_scenes_g():
    global g_scenes, g_scenes_names, g_renderers, g_scenes_loaded
    if not g_scenes_loaded:
        g_scenes, g_scenes_names, g_renderers = load_scenes(scenes_file, renderers_dir)
        g_scenes_loaded = True



def load_filters_g():
    global g_filters, g_filters_names, g_filters_versions, g_filters_loaded
    if not g_filters_loaded:
        g_filters, g_filters_names, g_filters_versions = load_filters(denoisers_dir)
        g_filters_loaded = True


def load_samplers_g():
    global g_samplers, g_samplers_names, g_samplers_versions, g_samplers_loaded
    if not g_samplers_loaded:
        g_samplers, g_samplers_names, g_samplers_versions = load_samplers(samplers_dir)
        g_samplers_loaded = True


def load_results_g():
    global g_filters, g_scenes, g_results_loaded
    if not g_results_loaded:
        load_filters_results(g_filters, g_scenes, current_slot_dir)
        g_results_loaded = True


def load_samplers_results_g():
    global g_samplers, g_scenes, g_samplers_results_loaded
    if not g_samplers_results_loaded:
        load_samplers_results(g_samplers, g_scenes, current_slot_dir)
        g_samplers_results_loaded = True


def save_scenes_file_g(scenes):
    save_scenes_file(scenes, current_slot_dir)


def save_filters_file_g(scenes, filters_ids):
    save_techniques_file(scenes, filters_ids, g_filters, os.path.join(current_slot_dir, 'filters.json'))


def save_samplers_file_g(scenes, samplers_ids):
    save_techniques_file(scenes, samplers_ids, g_samplers, os.path.join(current_slot_dir, 'samplers.json'))


def save_results_file_g(scenes, filters_ids):
    save_filters_result_file(current_slot_dir, scenes, g_filters, filters_ids)


def save_samplers_results_file_g(scenes, samplers_ids):
    save_samplers_result_file(current_slot_dir, scenes, g_samplers, samplers_ids)


#=============================================
#               Procedures                   #
#=============================================


def cmd_init(args):
    if not os.path.isdir(renderers_dir):
        os.mkdir(renderers_dir)

    if not os.path.isdir(denoisers_dir):
        os.mkdir(denoisers_dir)

    if not os.path.isdir(samplers_dir):
        os.mkdir(samplers_dir)

    if not os.path.isdir(configs_dir):
        os.mkdir(configs_dir)

    if not os.path.isdir(results_dir):
        new_slot_name = 'Results 1'
        if args.slot_name:
            new_slot_name = args.slot_name
        new_slot_path = os.path.join(results_dir, new_slot_name)
        os.makedirs(new_slot_path)
        os.symlink(new_slot_name, os.path.join(results_dir, '.current'))

    if not os.path.isdir(scenes_dir):
        if args.scenes_dir:
            if os.path.isdir(args.scenes_dir):
                os.symlink(os.path.abspath(args.scenes_dir), 'scenes')
            else:
                print('ERROR: \"{}\" is not an existing directory.\n'.format(args.scenes_dir))
        else:
            os.mkdir(scenes_dir)


def cmd_config(args):
    check = check_configs(configs_dir)
    if not check:
        return

    configs, current = get_configs(configs_dir, current_config)
    if not configs:
        print('No configuration found.')
        return

    print('{0}{1:<4s}{2:<16}{3:<20s}'.format(' ', 'Id', 'Creation Date', 'Name'))
    print('{0:75s}'.format('-'*75))
    for i, config in enumerate(configs):
        pretty_ctime = time.strftime('%d/%m/%Y', time.gmtime(config['ctime']))
        name = config['name']
        if current == config['name']:
            print('* {0:<3s}{1:<16}{2:<20s}'.format(str(i+1), pretty_ctime, name))
        else:
            print('  {0:<3s}{1:<16}{2:<20s}'.format(str(i+1), pretty_ctime, name))
    print('{0:75s}'.format('-'*75))


def cmd_config_new(args):
    check = check_scenes(scenes_dir) and check_filters(denoisers_dir) and check_samplers(samplers_dir)
    if not check:
        return

    new_config_file = os.path.join(configs_dir, args.name + '.json')
    if os.path.exists(new_config_file):
        print('ERROR: Config {} already exists.'.format(args.name))
        return
    if args.name[0] == '.':
        print('ERROR: Config name can not start with dot.')
        return

    load_scenes_g()
    scenes = []
    if args.scenes_all:
        scenes = g_scenes.values()
    else:
        scenes = scenesFromIds(args.scenes, g_scenes)

    load_filters_g()
    filters = []
    if args.filters_all:
        filters = g_filters.values()
    else:
        filters = techniqueVersionsFromIds(args.filters, g_filters_versions)

    load_samplers_g()
    samplers = []
    if args.samplers_all:
        samplers = g_samplers.values()
    else:
        samplers = techniqueVersionsFromIds(args.samplers, g_samplers_versions)

    spps = []
    if args.spps:
        spps = list(set(args.spps))
        spps.sort()
    else:
        spps = [2, 4, 8, 16]

    root_node = new_config(scenes, filters, samplers, spps)
    with open(new_config_file, 'w') as outfile:
        json.dump(root_node, outfile, indent=4)

    if args.select:
        if os.path.islink(current_config):
            os.unlink(current_config)
        os.symlink(args.name + '.json', current_config)


def cmd_config_select(args):
    check = check_configs(configs_dir)
    if not check:
        return

    configs, current = get_configs(configs_dir, current_config)
    if args.id < 1 or args.id > len(configs):
        print('ERROR: Invalid config ID.')
        return

    if os.path.islink(current_config):
        os.unlink(current_config)
    name = configs[args.id - 1]['name']
    os.symlink(name + '.json', current_config)
    print('Selected configuration: ' + name)


def cmd_config_show(args):
    check = check_configs(configs_dir)
    if not check:
        return

    if args.id is not None:
        configs, current = get_configs(configs_dir, current_config)
        if args.id < 1 or args.id > len(configs):
            print('ERROR: Invalid config ID.')
            return
        config_file = os.path.join(configs_dir, configs[args.id - 1]['name'] + '.json')
    else:
        config_file = current_config

    if not os.path.exists(config_file):
        print('Configuration not found.')
        return

    load_scenes_g()
    load_filters_g()
    load_samplers_g()
    config = Config(current_config, g_scenes_names, g_filters_names, g_samplers_names)
    config.print()


def cmd_config_add_scenes(args):
    check = check_configs(configs_dir) and \
            check_current_config(current_config) and \
            check_scenes(scenes_dir) and \
            check_filters(denoisers_dir) and \
            check_samplers(samplers_dir)
    if not check:
        return

    load_scenes_g()
    load_filters_g()
    load_samplers_g()

    scenes = []
    if args.all:
        scenes = g_scenes.values()
    else:
        scenes = scenesFromIds(args.scenes, g_scenes)

    spps = list(set(args.spps))
    spps.sort()

    config = Config(current_config, g_scenes_names, g_filters_names, g_samplers_names)
    for scene in scenes:
        config.add_scene(scene, spps)
    config.save()


def cmd_config_remove_scenes(args):
    check = check_configs(configs_dir) and \
            check_current_config(current_config) and \
            check_scenes(scenes_dir) and \
            check_filters(denoisers_dir) and \
            check_samplers(samplers_dir)
    if not check:
        return

    load_scenes_g()
    load_filters_g()
    load_samplers_g()

    scenes = scenesFromIds(args.scenes, g_scenes)
    config = Config(current_config, g_scenes_names, g_filters_names, g_samplers_names)
    for scene in scenes:
        config.remove_scene(scene)
    config.save()


def cmd_config_add_filters(args):
    check = check_configs(configs_dir) and \
            check_current_config(current_config) and \
            check_scenes(scenes_dir) and \
            check_filters(denoisers_dir) and \
            check_samplers(samplers_dir)
    if not check:
        return

    load_scenes_g()
    load_filters_g()
    load_samplers_g()

    filters = []
    if args.all:
        filters = g_filters_versions.values()
    else:
        filters = techniqueVersionsFromIds(args.filters, g_filters_versions)

    config = Config(current_config, g_scenes_names, g_filters_names, g_samplers_names)
    for f in filters:
        config.add_filter(f)
    config.save()


def cmd_config_remove_filters(args):
    check = check_configs(configs_dir) and \
            check_current_config(current_config) and \
            check_scenes(scenes_dir) and \
            check_filters(denoisers_dir) and \
            check_samplers(samplers_dir)
    if not check:
        return

    load_scenes_g()
    load_filters_g()
    load_samplers_g()

    filters = techniqueVersionsFromIds(args.filters, g_filters_versions)
    config = Config(current_config, g_scenes_names, g_filters_names, g_samplers_names)
    for f in filters:
        config.remove_filter(f)
    config.save()


def cmd_config_add_samplers(args):
    check = check_configs(configs_dir) and \
            check_current_config(current_config) and \
            check_scenes(scenes_dir) and \
            check_filters(denoisers_dir) and \
            check_samplers(samplers_dir)
    if not check:
        return

    load_scenes_g()
    load_filters_g()
    load_samplers_g()

    samplers = []
    if args.all:
        samplers = g_samplers_versions.values()
    else:
        samplers = techniqueVersionsFromIds(args.samplers, g_samplers_versions)

    config = Config(current_config, g_scenes_names, g_filters_names, g_samplers_names)
    for s in samplers:
        config.add_sampler(s)
    config.save()


def cmd_config_remove_samplers(args):
    check = check_configs(configs_dir) and \
            check_current_config(current_config) and \
            check_scenes(scenes_dir) and \
            check_filters(denoisers_dir) and \
            check_samplers(samplers_dir)
    if not check:
        return

    load_scenes_g()
    load_filters_g()
    load_samplers_g()

    samplers = techniqueVersionsFromIds(args.samplers, g_samplers_versions)
    config = Config(current_config, g_scenes_names, g_filters_names, g_samplers_names)
    for s in samplers:
        config.remove_sampler(s)
    config.save()


def cmd_config_add_spps(args):
    check = check_configs(configs_dir) and \
            check_current_config(current_config) and \
            check_scenes(scenes_dir) and \
            check_filters(denoisers_dir) and \
            check_samplers(samplers_dir)
    if not check:
        return

    load_scenes_g()
    load_filters_g()
    load_samplers_g()

    spps = set(args.spps)

    config = Config(current_config, g_scenes_names, g_filters_names, g_samplers_names)
    config_scenes = []
    if args.scenes:
        scenes = scenesFromIds(args.scenes, g_scenes)
        for s in config.scenes:
            if s.scene in scenes:
                config_scenes.append(s)
    else:
        config_scenes = config.scenes

    for s in config_scenes:
        s.spps = list(set(s.spps) | spps)
        s.spps.sort()
    config.save()


def cmd_config_remove_spps(args):
    check = check_configs(configs_dir) and \
            check_current_config(current_config) and \
            check_scenes(scenes_dir) and \
            check_filters(denoisers_dir) and \
            check_samplers(samplers_dir)
    if not check:
        return

    load_scenes_g()
    load_filters_g()
    load_samplers_g()

    spps = set(args.spps)

    config = Config(current_config, g_scenes_names, g_filters_names, g_samplers_names)
    config_scenes = []
    if args.scenes:
        scenes = scenesFromIds(args.scenes, g_scenes)
        for s in config.scenes:
            if s.scene in scenes:
                config_scenes.append(s)
    else:
        config_scenes = config.scenes

    for s in config_scenes:
        s.spps = list(set(s.spps) - spps)
        s.spps.sort()
    config.save()


def cmd_page_update(args):
    check = check_scenes(scenes_dir) and \
            check_filters(denoisers_dir) and \
            check_samplers(samplers_dir) and \
            check_results(results_dir) and \
            check_current_slot(current_slot_dir)
    if not check:
        return

    load_scenes_g()
    load_filters_g()
    load_samplers_g()
    load_results_g()
    load_samplers_results_g()

    if args.all:
        scenes = g_scenes
        filters_ids = g_filters_versions.keys()
        samplers_ids = g_samplers_versions.keys()
    else:
        check = check_configs(configs_dir) and \
                check_current_config(current_config)
        if not check:
            return
        config = Config(current_config, g_scenes_names, g_filters_names, g_samplers_names)
        scenes = {s.scene.id : s for s in config.scenes}
        filters_ids = [v.id for v in config.filter_versions]
        samplers_ids = [v.id for v in config.sampler_versions]

    save_scenes_file_g(scenes)
    save_filters_file_g(scenes, filters_ids)
    save_samplers_file_g(scenes, samplers_ids)
    save_results_file_g(scenes, filters_ids)
    save_samplers_results_file_g(scenes, samplers_ids)


def cmd_filters(args):
    if not check_filters(denoisers_dir):
        return

    load_filters_g()
    if not g_filters_versions:
        print('No filters found.')
        return

    print('{0:<5s}{1:<20s}{2:<10s}'.format('Id', 'Name', 'Status'))
    print('{0:75s}'.format('-'*75))
    for id, v in g_filters_versions.items():
        if v.status != 'ready' and args.ready:
            continue
        if v.tag == 'default':
            print('{0:<5s}{1:<20s}{2:<10s}'.format(str(id), v.technique.name, v.status))
        else:
            print('{0:<5s}{1:<20s}{2:<10s}'.format(str(id), v.technique.name + '-{}'.format(v.tag), v.status))
    print('{0:75s}'.format('-'*75))


def cmd_samplers(args):
    if not check_samplers(samplers_dir):
        return

    load_samplers_g()
    if not g_samplers_versions:
        print('No samplers found.')
        return

    print('{0:<5s}{1:<20s}{2:<10s}'.format('Id', 'Name', 'Status'))
    print('{0:75s}'.format('-'*75))
    for id, v in g_samplers_versions.items():
        if v.tag == 'default':
            print('{0:<5s}{1:<20s}{2:<10s}'.format(str(id), v.technique.name, v.status))
        else:
            print('{0:<5s}{1:<20s}{2:<10s}'.format(str(id), v.technique.name + '-{}'.format(v.tag), v.status))
    print('{0:75s}'.format('-'*75))


def cmd_filter_info(args):
    if not check_filters(denoisers_dir):
        return

    load_filters_g()
    filters = techniqueVersionsFromIds(args.filters, g_filters_versions)

    for f in filters:
        print('Name:      {}'.format(f.technique.name))
        print('Full name: {}'.format(f.technique.full_name))
        print('Comment:   {}'.format(f.technique.comment))
        print('Citation:  {}'.format(f.technique.citation))
        print('Versions:')
        for version in f.technique.versions:
            print('{}'.format(version.id))
            print('    Name:    {}'.format(version.tag))
            print('    Message: {}'.format(version.message))


def cmd_sampler_info(args):
    if not check_samplers(samplers_dir):
        return

    load_samplers_g()
    samplers = techniqueVersionsFromIds(args.samplers, g_samplers_versions)

    for f in samplers:
        print('Name:      {}'.format(f.technique.name))
        print('Full name: {}'.format(f.technique.full_name))
        print('Comment:   {}'.format(f.technique.comment))
        print('Citation:  {}'.format(f.technique.citation))
        print('Versions:')
        for version in f.technique.versions:
            print('{}'.format(version.id))
            print('    Name:    {}'.format(version.tag))
            print('    Message: {}'.format(version.message))


def cmd_scenes(args):
    if args.set:
        if os.path.isdir(args.set):
            if os.path.islink(scenes_dir):
                os.unlink(scenes_dir)
            elif os.path.exists(scenes_dir):
                print('ERROR: scenes folder already exists.')
                return
            os.symlink(os.path.abspath(args.set), 'scenes')
            print('Scenes path set.\n')
            return
        else:
            print('ERROR: \"{}\" is not an existing directory.\n'.format(args.set))
            return

    needs_update = False
    if not check_scenes(scenes_dir):
        return
    elif not os.path.exists(scenes_file):
        needs_update = True

    if args.update or needs_update:
        print('Updating scenes cache file...')
        scan_scenes(scenes_dir)
        print('Done.\n')

    ready_only = args.ready

    load_scenes_g()
    scenes = g_scenes
    if ready_only:
        scenes = {sid: g_scenes[sid] for sid in g_scenes.keys() if g_scenes[sid].renderer.is_ready}

    if not scenes:
        print('No scenes.')
        return

    print('{0:<5s}{1:<50s}{2:<20s}'.format('Id', 'Name', 'Renderer'))
    print('{0:75s}'.format('-'*75))
    for sid, scene in scenes.items():
        print('{0:<5s}{1:<50s}{2:<20s}'.format(str(sid), scene.name, scene.renderer.name))
    print('{0:75s}'.format('-'*75))


def cmd_run(args):
    check = check_configs(configs_dir) and \
            check_current_config(current_config) and \
            check_scenes(scenes_dir) and \
            check_filters(denoisers_dir) and \
            check_samplers(samplers_dir) and \
            check_results(results_dir) and \
            check_current_slot(current_slot_dir)
    if not check:
        return

    load_scenes_g()
    load_filters_g()
    load_samplers_g()
    config_filename = '/tmp/benchmark_config.json'
    config = write_tmp_config(config_filename, current_config, renderers_dir, g_renderers, scenes_dir, g_scenes_names)
    if not config:
        print('Nothing to run.')
    print('Running configuration \'' + current_config_name(current_config) + '\'\n')

    benchmark_exec = os.path.join(install_prefix_dir, 'bin/fbksd-benchmark')
    run_techniques(
        benchmark_exec,
        denoisers_dir,
        config['filters'],
        g_filters_names,
        os.path.join(current_slot_dir, 'denoisers'),
        config_filename,
        args.overwrite
    )

    run_techniques(
        benchmark_exec,
        samplers_dir,
        config['samplers'],
        g_samplers_names,
        os.path.join(current_slot_dir, 'samplers'),
        config_filename,
        args.overwrite
    )

    print('Benchmark finished. Run \'fbksd results compute\' to compute result errors.')


# computes errors for each result image and saves a corresponding log file for each one.
# overwrite logs that are older then the corresponding result images
def cmd_results_compute(args):
    check = check_scenes(scenes_dir) and \
            check_filters(denoisers_dir) and \
            check_samplers(samplers_dir) and \
            check_results(results_dir) and \
            check_current_slot(current_slot_dir)
    if not check:
        return

    load_scenes_g()
    load_filters_g()
    load_samplers_g()

    compare_exec = os.path.join(install_prefix_dir, 'bin/fbksd-compare')
    exr2png_exec = os.path.join(install_prefix_dir, 'bin/fbksd-exr2png')

    if args.all:
        scenes = g_scenes_names
        # NOTE: The compare_techniques_results function computes all results in the slot
        # for the given scenes when the versions arg is empty.
        filters = None
        samplers = None
    else:
        check = check_configs(configs_dir) and \
                check_current_config(current_config)
        if not check:
            return
        config = Config(current_config, g_scenes_names, g_filters_names, g_samplers_names)
        scenes = {s.get_name():s for s in config.scenes}
        filters = {v.get_name():v for v in config.filter_versions}
        samplers = {v.get_name():v for v in config.sampler_versions}

    print('Comparing results...')
    compare_techniques_results(
        os.path.join(current_slot_dir, 'denoisers'),
        scenes,
        filters,
        scenes_dir,
        exr2png_exec,
        compare_exec,
        args.overwrite
    )

    compare_techniques_results(
        os.path.join(current_slot_dir, 'samplers'),
        scenes,
        samplers,
        scenes_dir,
        exr2png_exec,
        compare_exec,
        args.overwrite
    )

    print('Updating visualization page cache...')
    cmd_page_update(args)


def cmd_results_show(args):
    check = check_scenes(scenes_dir) and \
            check_filters(denoisers_dir) and \
            check_samplers(samplers_dir) and \
            check_results(results_dir) and \
            check_current_slot(current_slot_dir)
    if not check:
        return

    load_scenes_g()
    load_filters_g()
    load_results_g()
    load_samplers_g()
    load_samplers_results_g()

    metrics = []
    if args.mse:
        metrics.append('mse')
    if args.psnr:
        metrics.append('psnr')
    if args.ssim:
        metrics.append('ssim')
    if args.rmse:
        metrics.append('rmse')
    if not metrics:
        metrics = ['mse', 'psnr', 'ssim', 'rmse']

    if args.all:
        scenes = g_scenes.values()
        filters = g_filters_versions.values()
        samplers = g_samplers_versions.values()
    else:
        check = check_configs(configs_dir) and \
                check_current_config(current_config)
        if not check:
            return
        config = Config(current_config, g_scenes_names, g_filters_names, g_samplers_names)
        scenes = config.scenes
        filters = config.filter_versions
        samplers = config.sampler_versions

    if filters:
        print('DENOISERS')
        print_results(filters, scenes, metrics)
    if samplers:
        print('SAMPLERS')
        print_results(samplers, scenes, metrics)


def cmd_results_rank(args):
    check = check_scenes(scenes_dir) and \
            check_filters(denoisers_dir) and \
            check_samplers(samplers_dir) and \
            check_results(results_dir) and \
            check_current_slot(current_slot_dir)
    if not check:
        return

    load_scenes_g()
    load_filters_g()
    load_results_g()
    load_samplers_g()
    load_samplers_results_g()

    metrics = []
    if args.mse:
        metrics.append(ErrorMetric('mse'))
    if args.psnr:
        metrics.append(ErrorMetric('psnr', lower_is_better=False))
    if args.ssim:
        metrics.append(ErrorMetric('ssim', lower_is_better=False))
    if args.rmse:
        metrics.append(ErrorMetric('rmse'))
    if not metrics:
        metrics = [ErrorMetric('mse'),
            ErrorMetric('psnr', lower_is_better=False),
            ErrorMetric('ssim', lower_is_better=False),
            ErrorMetric('rmse')]

    if args.all:
        scenes = g_scenes.values()
        filters = g_filters_versions.values()
        samplers = g_samplers_versions.values()
    else:
        check = check_configs(configs_dir) and \
                check_current_config(current_config)
        if not check:
            return
        config = Config(current_config, g_scenes_names, g_filters_names, g_samplers_names)
        scenes = [s.scene for s in config.scenes]
        filters = config.filter_versions
        samplers = config.sampler_versions

    rank_techniques('Denoisers Ranking', scenes, filters, metrics)
    rank_techniques('Samplers Ranking', scenes, samplers, metrics)


def cmd_results_export_csv(args):
    check = check_scenes(scenes_dir) and \
            check_filters(denoisers_dir) and \
            check_samplers(samplers_dir) and \
            check_results(results_dir) and \
            check_current_slot(current_slot_dir)
    if not check:
        return

    load_scenes_g()
    load_filters_g()
    load_samplers_g()
    load_results_g()
    load_samplers_results_g()

    if args.all:
        scenes = g_scenes.values()
        filters = g_filters_versions.values()
        samplers = g_samplers_versions.values()
    else:
        check = check_configs(configs_dir) and \
                check_current_config(current_config)
        if not check:
            return
        config = Config(current_config, g_scenes_names, g_filters_names, g_samplers_names)
        scenes = [s.scene for s in config.scenes]
        filters = config.filter_versions
        samplers = config.sampler_versions

    metrics = []
    if args.mse:
        metrics.append('mse')
    if args.psnr:
        metrics.append('psnr')
    if args.ssim:
        metrics.append('ssim')
    if args.rmse:
        metrics.append('rmse')
    if not metrics:
        metrics = ['mse', 'psnr', 'ssim', 'rmse']

    mse_scale = 1.0
    if args.mse_scale:
        mse_scale = args.mse_scale

    rmse_scale = 1.0
    if args.rmse_scale:
        rmse_scale = args.rmse_scale

    if filters:
        print('Filters')
        print_csv(scenes, filters, metrics, mse_scale, rmse_scale)
    if samplers:
        print('Samplers')
        print_csv(scenes, samplers, metrics, mse_scale, rmse_scale)


def cmd_page_export(args):
    check = check_scenes(scenes_dir) and \
            check_filters(denoisers_dir) and \
            check_samplers(samplers_dir) and \
            check_results(results_dir) and \
            check_current_slot(current_slot_dir)
    if not check:
        return

    if os.path.isdir(args.dest):
        if args.overwrite:
            print('Overwriting existing files.')
        else:
            print('Destination already exists (see option \'--overwrite\').')
            return

    load_scenes_g()
    load_filters_g()
    load_samplers_g()
    load_results_g()
    load_samplers_results_g()

    if args.all:
        scenes = g_scenes.values()
        filters = g_filters_versions.values()
        samplers = g_samplers_versions.values()
        scene_config_spps = None
    else:
        check = check_configs(configs_dir) and \
                check_current_config(current_config)
        if not check:
            return
        config = Config(current_config, g_scenes_names, g_filters_names, g_samplers_names)
        scenes = [s.scene for s in config.scenes]
        filters = config.filter_versions
        samplers = config.sampler_versions
        scene_config_spps = {s.scene.id : s.spps for s in config.scenes}

    def copy_results(versions, scene_config_spps, tech_path):
        for version in versions:
            for result in version.results:
                if scene_config_spps:
                    if result.scene.id not in scene_config_spps:
                        continue
                    spps = scene_config_spps[result.scene.id]
                    if result.spp not in spps:
                        continue
                path = os.path.join(tech_path, version.technique.name, version.tag, result.scene.name)
                dest_path = os.path.join(args.dest, 'data', path)
                if not os.path.exists(dest_path):
                    os.makedirs(dest_path)
                suffixes = [
                    os.path.join(path, '{}_0.png'.format(result.spp)),
                    os.path.join(path, '{}_0_mse_map.png'.format(result.spp)),
                    os.path.join(path, '{}_0_ssim_map.png'.format(result.spp)),
                    os.path.join(path, '{}_0_rmse_map.png'.format(result.spp)),
                ]
                for suffix in suffixes:
                    img = os.path.join(current_slot_dir, suffix)
                    if not os.path.isfile(img):
                        continue
                    print('{} -> {}'.format(img, os.path.join(args.dest, 'data', suffix)))
                    shutil.copyfile(img, os.path.join(args.dest, 'data', suffix))

    # copy filters and samplers result images
    copy_results(filters, scene_config_spps, 'denoisers')
    copy_results(samplers, scene_config_spps, 'samplers')
    # copy data files
    data_dest = os.path.join(args.dest, 'data')
    shutil.copyfile(os.path.join(current_slot_dir, 'scenes.json'), os.path.join(data_dest, 'scenes.json'))
    shutil.copyfile(os.path.join(current_slot_dir, 'filters.json'), os.path.join(data_dest, 'filters.json'))
    shutil.copyfile(os.path.join(current_slot_dir, 'results.json'), os.path.join(data_dest, 'results.json'))
    shutil.copyfile(os.path.join(current_slot_dir, 'samplers.json'), os.path.join(data_dest, 'samplers.json'))
    shutil.copyfile(os.path.join(current_slot_dir, 'samplers_results.json'), os.path.join(data_dest, 'samplers_results.json'))
    # copy scenes images
    for scene in scenes:
        orig_gt = os.path.join(scenes_dir, scene.get_reference())
        orig_png = os.path.splitext(orig_gt)[0] + '.png'
        orig_thumb = os.path.splitext(orig_gt)[0] + '_thumb256.jpg'
        dest_gt = os.path.join(args.dest, 'scenes', scene.get_reference())
        dest_path = os.path.dirname(dest_gt)
        dest_png = os.path.splitext(dest_gt)[0] + '.png'
        dest_thumb = os.path.splitext(dest_gt)[0] + '_thumb256.jpg'
        if not os.path.exists(dest_path):
            os.makedirs(dest_path)
        print('{} -> {}'.format(orig_png, dest_png))
        shutil.copyfile(orig_png, dest_png)
        print('{} -> {}'.format(orig_thumb, dest_thumb))
        shutil.copyfile(orig_thumb, dest_thumb)
    # copy page files
    if not args.results_only:
        src = os.path.join(install_prefix_dir, 'share/fbksd/page/')
        dest = args.dest if args.dest[-1] == '/' else args.dest + '/'
        subprocess.run(['rsync', '-a', '--exclude', 'data', '--exclude', 'scenes', src, dest])


def cmd_slots(args):
    if not check_results(results_dir):
        return

    slots, current_slot = get_slots(results_dir)
    print('{0}{1:<4s}{2:<16}{3:<20s}'.format(' ', 'Id', 'Creation Date', 'Name'))
    print('{0:75s}'.format('-'*75))
    for i, slot in enumerate(slots):
        pretty_ctime = time.strftime('%d/%m/%Y', time.gmtime(slot['ctime']))
        name = os.path.basename(slot['name'])
        if current_slot == slot['name']:
            print('* {0:<4s}{1:<16}{2:<20s}'.format(str(i+1), pretty_ctime, name))
        else:
            print('  {0:<4s}{1:<16}{2:<20s}'.format(str(i+1), pretty_ctime, name))
    print('{0:75s}'.format('-'*75))


def cmd_slots_new(args):
    if not check_results(results_dir):
        return

    invalid_chars = ['.', '/', '\\']
    for char in invalid_chars:
        if char in args.name:
            print('ERROR: Slot name can not contain \'{}\'.'.format(char))
            return

    new_path = os.path.join(results_dir, args.name)
    if os.path.exists(new_path):
        print('ERROR: Slot {} already exists.'.format(args.name))
        return

    if not os.path.islink(current_slot_dir):
        print('ERROR: {} is not a symlink.'.format(current_slot_dir))
        return

    os.mkdir(new_path)
    if args.select:
        if os.path.islink(current_slot_dir):
            os.unlink(current_slot_dir)
        os.symlink(args.name, current_slot_dir)


def cmd_slots_select(args):
    if not check_results(results_dir):
        return

    slots, current_slot = get_slots(results_dir)
    if args.id < 1 or args.id > len(slots):
        print('ERROR: Invalid ID.')
        return

    if os.path.isdir(current_slot_dir):
        if not os.path.islink(current_slot_dir):
            print('ERROR: {} is not a symlink.'.format(current_slot_dir))
            return
        os.unlink(current_slot_dir)
    os.symlink(slots[args.id - 1]['name'], current_slot_dir)


def cmd_serve(args):
    check = check_results(results_dir) and \
            check_current_slot(current_slot_dir) and \
            check_scenes(scenes_dir)
    if not check:
        return

    stdout = sys.stderr
    if not args.no_update:
        print('Updating visualization page cache...')
        cmd_page_update(args)
    orig_page = os.path.join(install_prefix_dir, 'share/fbksd/page/')
    subprocess.run(['rsync', '-a', '--delete', orig_page, page_dir])
    try:
        os.chdir(page_dir)
        Handler = http.server.SimpleHTTPRequestHandler
        httpd = socketserver.TCPServer(('', args.port), Handler)
        print('serving at port {}. Press Ctrl-C to finish.'.format(args.port))
        f = open(os.devnull, 'w')
        sys.stderr = f
        httpd.serve_forever()
    except KeyboardInterrupt:
        sys.stderr = stdout
        print('\b\bfinished.')



#=============================================
#                    Main                    #
#=============================================
if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog='fbksd', description='fbksd system cli interface.')
    parser.add_argument('--version', action='version', version='%(prog)s version 1.0.0')
    subparsers = parser.add_subparsers(title='subcommands')

    # init
    parserInit = subparsers.add_parser('init', formatter_class=RawTextHelpFormatter,
        help='Initialize a workspace in the current directory.',
        description=
            'Initialize a workspace in the current directory.\n\n'
            'A workspace is the folder where all the renderers, techniques, and benchmarking data is kept.\n'
            'You should always call \'fbksd\' having a workspace as working directory.')
    parserInit.add_argument('--slot-name', help='Name of the result slot (default: \"Results 1\")')
    parserInit.add_argument('--scenes-dir', help='Path to a scenes folder. A link to that folder is created in the current directory.')
    parserInit.set_defaults(func=cmd_init)

    # config
    parserConfig = subparsers.add_parser('config', formatter_class=RawTextHelpFormatter,
        help="Manage configurations.",
        description=
            'A configuration is a set of techniques and scenes to be executed. Each technique will be executed\n'
            'for each scene with a set of samples-per-pixel (spp), which can also be configured.\n'
            'Subsequent commands like \'run\', \'results\', etc. will act on the current configuration.\n\n'
            'Each configuration has a corresponding file in the \'configs\' folder. You can edit them directly.')
    parserConfig.set_defaults(func=cmd_config)
    configSubparsers = parserConfig.add_subparsers(title='subcommands')
    ## config new
    parserConfigNew = configSubparsers.add_parser('new', formatter_class=RawTextHelpFormatter,
         help='Creates a new configuration.',
         description=
            'Creates a new configuration with the given name, containing the scenes, techniques,\n'
            'and spps provided using the corresponding options.\n\n'
            'The created config is not selected by default. You can select it using the \'--select\' option.')
    parserConfigNew.add_argument('name', metavar='NAME', help='Configuration name.')
    parserConfigNew.add_argument('--scenes', nargs='+', type=int, metavar='SCENE_ID', help='Scenes to be included.')
    parserConfigNew.add_argument('--scenes-all', action='store_true', help='Include all scenes.')
    parserConfigNew.add_argument('--filters', nargs='+', type=int, metavar='FILTER_ID', help='Filters to be included.')
    parserConfigNew.add_argument('--filters-all', action='store_true', help='Include all filters.')
    parserConfigNew.add_argument('--samplers', nargs='+', type=int, metavar='SAMPLER_ID', help='Samplers to be included.')
    parserConfigNew.add_argument('--samplers-all', action='store_true', help='Include all samplers.')
    parserConfigNew.add_argument('--spps', metavar='SPP', type=int, nargs='+', help='List of spps to use.')
    parserConfigNew.add_argument('--select', action='store_true', help='Select the created config.')
    parserConfigNew.set_defaults(func=cmd_config_new)
    ## config select
    parserConfigSelect = configSubparsers.add_parser('select', formatter_class=RawTextHelpFormatter,
        help='Select a configuration.',
        description=
            'Select a configuration given its ID.\n\n'
            'The selected configuration is used by subsequent commands like \'run\' and \'results\'.')
    parserConfigSelect.add_argument('id', type=int, metavar='CONFIG_ID', help='Id of the desired configuration.')
    parserConfigSelect.set_defaults(func=cmd_config_select)
    ## config show
    parserConfigShow = configSubparsers.add_parser('show', formatter_class=RawTextHelpFormatter,
        help='Show configuration details.',
        description=
            'Show the contents of a configuration\'s config file. If no ID is given, the current config is shown.')
    parserConfigShow.add_argument('--id', type=int, metavar='CONFIG_ID', help='Id of the desired configuration.')
    parserConfigShow.set_defaults(func=cmd_config_show)
    ## config add-scenes
    parserConfigAddScenes = configSubparsers.add_parser('add-scenes', formatter_class=RawTextHelpFormatter,
         help='Add scenes to the current config.',
         description='Add scenes to the current config.')
    parserConfigAddScenes.add_argument('scenes', nargs='*', type=int, metavar='SCENE_ID', help='Scene ID.')
    parserConfigAddScenes.add_argument('--spps', metavar='SPP', type=int, nargs='+', required=True, help='spps used for the added scenes.')
    parserConfigAddScenes.add_argument('--all', action='store_true', help='Add all available scenes.')
    parserConfigAddScenes.set_defaults(func=cmd_config_add_scenes)
    ## config rm-scenes
    parserConfigRemoveScenes = configSubparsers.add_parser('rm-scenes', formatter_class=RawTextHelpFormatter,
         help='Remove scenes from the current config.',
         description='Remove scenes from the current config.')
    parserConfigRemoveScenes.add_argument('scenes', nargs='+', type=int, metavar='SCENE_ID', help='Scene ID.')
    parserConfigRemoveScenes.set_defaults(func=cmd_config_remove_scenes)
    ## config add-filters
    parserConfigAddFilters = configSubparsers.add_parser('add-filters', formatter_class=RawTextHelpFormatter,
         help='Add filters to the current config.',
         description='Add filters to the current config.')
    parserConfigAddFilters.add_argument('filters', nargs='*', type=int, metavar='FILTER_ID', help='Filter ID.')
    parserConfigAddFilters.add_argument('--all', action='store_true', help='Add all available filters.')
    parserConfigAddFilters.set_defaults(func=cmd_config_add_filters)
    ## config rm-filters
    parserConfigRemoveFilters = configSubparsers.add_parser('rm-filters', formatter_class=RawTextHelpFormatter,
         help='Remove filters from the current config.',
         description='Remove filters from the current config.')
    parserConfigRemoveFilters.add_argument('filters', nargs='+', type=int, metavar='FILTER_ID', help='Filter ID.')
    parserConfigRemoveFilters.set_defaults(func=cmd_config_remove_filters)
    ## config add-samplers
    parserConfigAddSamplers = configSubparsers.add_parser('add-samplers', formatter_class=RawTextHelpFormatter,
         help='Add samplers to the current config.',
         description='Add samplers to the current config.')
    parserConfigAddSamplers.add_argument('samplers', nargs='*', type=int, metavar='FILTER_ID', help='Sampler ID.')
    parserConfigAddSamplers.add_argument('--all', action='store_true', help='Add all available samplers.')
    parserConfigAddSamplers.set_defaults(func=cmd_config_add_samplers)
    ## config rm-samplers
    parserConfigRemoveSamplers = configSubparsers.add_parser('rm-samplers', formatter_class=RawTextHelpFormatter,
         help='Remove samplers from the current config.',
         description='Remove samplers from the current config.')
    parserConfigRemoveSamplers.add_argument('samplers', nargs='+', type=int, metavar='FILTER_ID', help='Sampler ID.')
    parserConfigRemoveSamplers.set_defaults(func=cmd_config_remove_samplers)
    ## config add-spps
    parserConfigAddSpps = configSubparsers.add_parser('add-spps', formatter_class=RawTextHelpFormatter,
         help='Add spps to the current config.',
         description=
            'Add spps to the current config.\n\n'
            'By default, the spps are added to all scenes in the config. You can add to specific scenes using\n'
            'the \'--scenes\' option.')
    parserConfigAddSpps.add_argument('spps', nargs='+', type=int, metavar='SPP', help='spp value.')
    parserConfigAddSpps.add_argument('--scenes', metavar='SCENE_ID', type=int, nargs='+',
        help='Add the spps to specific scenes.')
    parserConfigAddSpps.set_defaults(func=cmd_config_add_spps)
    ## config rm-spps
    parserConfigRemoveSpps = configSubparsers.add_parser('rm-spps', formatter_class=RawTextHelpFormatter,
         help='Remove spps from the current config.',
         description=
            'Remove spps from the current config.\n\n'
            'By default, the spps are removed from all scenes in the config. You can remove from specific scenes using\n'
            'the \'--scenes\' option.')
    parserConfigRemoveSpps.add_argument('spps', nargs='+', type=int, metavar='SCENE_ID', help='spps.')
    parserConfigRemoveSpps.add_argument('--scenes', metavar='SCENE_ID', type=int, nargs='+',
        help='Remove the spps from specific scenes.')
    parserConfigRemoveSpps.set_defaults(func=cmd_config_remove_spps)

    # filters
    parserFilters = subparsers.add_parser('filters', formatter_class=RawTextHelpFormatter,
        help='List all filters.', description='List all filters.')
    parserFilters.add_argument('--ready', action='store_true', help='List only ready (compiled) denoisers.')
    parserFilters.set_defaults(func=cmd_filters)

    # samplers
    parserSamplers = subparsers.add_parser('samplers', formatter_class=RawTextHelpFormatter,
        help='List all samplers.', description="List all samplers.")
    parserSamplers.add_argument('--ready', action='store_true', help='List only ready (compiled) samplers.')
    parserSamplers.set_defaults(func=cmd_samplers)

    # scenes
    parserScenes = subparsers.add_parser('scenes', formatter_class=RawTextHelpFormatter,
        help='Manage the scenes cache.',
        description=
            'Without options, this command list all scenes.\n\n'
            'The scenes are read from the a cache file located in the \'scenes\' folder. This file is generated automatically,\n'
            'by scanning the \'scenes\' folder. You can update the cache using the \'--update\' option.\n\n'
            'If you keep you scenes folder somewhere else, you can create a link for it in the workspace using the\n'
             '\'--set\' option.')
    parserScenes.add_argument('--set', metavar='SCENES_DIR', help='Create/change the \'scenes\' link to the given scenes folder.')
    parserScenes.add_argument('--update', action='store_true', help='Re-scan the scenes directory and re-generate the scenes cache file.')
    parserScenes.add_argument('--ready', action='store_true', help='List only scenes from ready renderers.')
    parserScenes.set_defaults(func=cmd_scenes)

    # filter-info
    parserFilterInfo = subparsers.add_parser('filter-info', formatter_class=RawTextHelpFormatter,
        help='Show details about filters.', description='Show details about filters.')
    parserFilterInfo.add_argument('filters', metavar='FILTER_ID', type=int, nargs='+', help='Filter ID.')
    parserFilterInfo.set_defaults(func=cmd_filter_info)
    # sampler-info
    parserSamplerInfo = subparsers.add_parser('sampler-info', formatter_class=RawTextHelpFormatter,
        help='Shows details about samplers.', description='Shows details about samplers.')
    parserSamplerInfo.add_argument('samplers', metavar='SAMPLER_ID', type=int, nargs='+', help='Sampler ID.')
    parserSamplerInfo.set_defaults(func=cmd_sampler_info)

    # run
    parserRun = subparsers.add_parser('run', formatter_class=RawTextHelpFormatter,
        help='Run the benchmark with the current configuration.',
        description='Run the benchmark with the current configuration.')
    parserRun.set_defaults(func=cmd_run)
    parserRun.add_argument('--overwrite', action='store_true', help='Overwrites previous results.')

    # results
    parserResults = subparsers.add_parser('results', help='Manipulate results.')
    resultsSubparsers = parserResults.add_subparsers(title='results subcommands')
    ## results compute
    parserResultsCompute = resultsSubparsers.add_parser('compute',formatter_class=RawTextHelpFormatter,
        help='Compute errors for the saved result images.',
        description=
            'Compute errors for the saved result images.\n\n'
            'This action should be performed after running a benchmark.\n'
            'By default, only results for the current config are computed. You can compute all the results\n'
            'in the slot using the \'--all\' option.')
    parserResultsCompute.add_argument('--all', action='store_true', help='Compute all results available in the slot.')
    parserResultsCompute.add_argument('--overwrite', action='store_true', dest='overwrite', help='Overwrite previous results.')
    parserResultsCompute.set_defaults(func=cmd_results_compute)
    ## results show
    parserResultsShow = resultsSubparsers.add_parser('show', formatter_class=RawTextHelpFormatter,
        help='Show results.',
        description=
            'Show results.\n\n'
            'By default, only errors metrics for the current config are shown. You can show all the results\n'
            'in the slot using the \'--all\' options, and also specify individual error metrics.')
    parserResultsShow.add_argument('--all', action='store_true', help='Show all results available in the slot.')
    parserResultsShow.add_argument('--ssim', action='store_true', help='Show ssim error.')
    parserResultsShow.add_argument('--mse', action='store_true', help='Shows mse error.')
    parserResultsShow.add_argument('--psnr', action='store_true', help='Shows psnr error.')
    parserResultsShow.add_argument('--rmse', action='store_true', help='Shows rmse error.')
    parserResultsShow.set_defaults(func=cmd_results_show)
    ## results rank
    parserResultsRank = resultsSubparsers.add_parser('rank', formatter_class=RawTextHelpFormatter,
        help='Show the techniques ranking.',
        description=
            'The ranking is computed by ordering the techniques for each scene/spp/metric and averaging\n'
            'all ranks for each technique.\n\n'
            'Techniques with larger rank value are worst, while techniques with rank closer to 1.0 are better.')
    parserResultsRank.add_argument('--all', action='store_true', help='Rank all results available in the workspace.')
    parserResultsRank.add_argument('--ssim', action='store_true', help='Shows ssim error.')
    parserResultsRank.add_argument('--mse', action='store_true', help='Shows mse error.')
    parserResultsRank.add_argument('--psnr', action='store_true', help='Shows psnr error.')
    parserResultsRank.add_argument('--rmse', action='store_true', help='Shows rmse error.')
    parserResultsRank.set_defaults(func=cmd_results_rank)
    ## result print-csv
    parserResultsExportCSV = resultsSubparsers.add_parser('print-csv', formatter_class=RawTextHelpFormatter,
        help='Print CSV tables with the results.',
        description=
            'Print CSV tables with the results.\n\n'
            'The results are printed in the folowing layout (using \',\' to separete cells):\n\n'
            '│         │       │ technique 1 │          │ technique 2 │          │\n'
            '│         │       │ metric 1    │ metric 2 │ metric 1    │ metric 2 │\n'
            '│ scene 1 │ spp 1 │    e1       │    e2    │    e3       │    e4    │\n'
            '│         │ spp 2 │    e5       │    e6    │    e7       │    e8    │\n'
            '│ scene 2 │ spp 1 │    e9       │    e10   │    e11      │    e12   │\n'
            '│         │ spp 2 │    e13      │    e14   │    e15      │    e16   │\n\n'
            'By default, only results for the current config are included.\n'
            'You can use the option \'--all\' to include all results available in the slot.')
    parserResultsExportCSV.add_argument('--all', action='store_true', help='Include all available results.')
    parserResultsExportCSV.add_argument('--ssim', action='store_true', help='Include ssim error.')
    parserResultsExportCSV.add_argument('--mse', action='store_true', help='Includes mse error.')
    parserResultsExportCSV.add_argument('--psnr', action='store_true', help='Includes psnr error.')
    parserResultsExportCSV.add_argument('--rmse', action='store_true', help='Includes rmse error.')
    parserResultsExportCSV.add_argument('--mse-scale', type=float, dest='mse_scale', help='Scale applied to the mse values.')
    parserResultsExportCSV.add_argument('--rmse-scale', type=float, dest='rmse_scale', help='Scale applied to the rmse values.')
    parserResultsExportCSV.set_defaults(func=cmd_results_export_csv)

    # slots
    parserSlots = subparsers.add_parser('slots', help='Manage results slots.')
    parserSlots.set_defaults(func=cmd_slots)
    slotsSubparsers = parserSlots.add_subparsers(title='slots subcommands')
    ## slots new
    parserSlotsNew = slotsSubparsers.add_parser('new', formatter_class=RawTextHelpFormatter,
        help='Create a new results slot.',
        description=
            'Create a new results slot.\n\n'
            'A slot is basically a folder in the \'<workspace>/results\' directory where all results data\n'
            '(images, error logs, etc) will be saved.'
            'The created slot is not selected by default. You can select it using the \'--select\' option.')
    parserSlotsNew.add_argument('name', metavar='NAME', help='New slot name.')
    parserSlotsNew.add_argument('--select', action='store_true', help='Select the created slot.')
    parserSlotsNew.set_defaults(func=cmd_slots_new)
    ## slots select
    parserSlotsSelect = slotsSubparsers.add_parser('select', help='Select a slot.')
    parserSlotsSelect.add_argument('id', type=int, metavar='SLOT_ID', help='Id of the desired slot.')
    parserSlotsSelect.set_defaults(func=cmd_slots_select)

    # page
    parserPage = subparsers.add_parser('page', formatter_class=RawTextHelpFormatter,
        help="Manage the visualization page.",
        description=
            'Once you have run a benchmark (\'fbksd run\') and computed the result errors (\'fbksd results compute\'),\n'
            'you can visualize the results using the included visualization page.')
    pageSubparsers = parserPage.add_subparsers(title='subcommands')
    pageSubparsers.required = True
    pageSubparsers.dest = 'subcommand'
    ## page serve
    parserServe = pageSubparsers.add_parser('serve', formatter_class=RawTextHelpFormatter,
        help='Serve the visualization page on a given port (use Ctrl-C to exit).',
        description=
            'Serve the visualization page on a given port (use Ctrl-C to exit).\n\n'
            'By default, a \'page update\' command is performed before serving the page.\n'
            'You can disable the update by using the \'--no-update\' option.')
    parserServe.add_argument('--port', help='Port number (default is 8000).', type=int, default=8000)
    parserServe.add_argument('--no-update', action='store_true', help='Don\'t call the \'page update\' command.')
    parserServe.add_argument('--all', action='store_true', help='Passes the \'--all\' option to the \'page update\' command.')
    parserServe.set_defaults(func=cmd_serve)
    ## page export
    parserPageExport = pageSubparsers.add_parser('export', formatter_class=RawTextHelpFormatter,
        help='Export the results and the visualization page.',
        description=
            'Export the results and the visualization page.\n\n'
            'The exported page can be statically served by any webserver.\n'
            'By default, only results for the current config are included.\n'
            'You can use the option \'--all\' to include all results available in the slot.')
    parserPageExport.add_argument('dest', metavar='DEST', help='Destination folder.')
    parserPageExport.add_argument('--all', action='store_true', help='Export all available results.')
    parserPageExport.add_argument('--overwrite', action='store_true', help='Overwrites DEST if it already exists.')
    parserPageExport.add_argument('--results-only', action='store_true', help='Exports only the results (not the html/javascrip/etc. page files.)')
    parserPageExport.set_defaults(func=cmd_page_export)
    ## page update
    parserPageUpdate = pageSubparsers.add_parser('update', formatter_class=RawTextHelpFormatter,
        help='Update the cache used by the visualization page.',
        description=
            'Update the cache used by the visualization page.\n\n'
            'The update consists in scanning all computed results and generating the set of files used by\n'
            'the visualization page.\n'
            'By default, only results for the current config are included.\n'
            'You can use the option \'--all\' to include all results available in the slot.\n\n'
            'This action is performed automatically when you run a \'serve\' or \'results compute\'')
    parserPageUpdate.add_argument('--all', action='store_true', help='Include all available results.')
    parserPageUpdate.set_defaults(func=cmd_page_update)

    args = parser.parse_args()
    if hasattr(args, 'func'):
        args.func(args)
    else:
        parser.print_help()
