#! /usr/bin/env python
# encoding: utf-8

import waflib

APPNAME = 'libduplex'
VERSION = '0.0.1'

HEADERS = [
    'duplex.h',
    'channel.h',
    'error.h',
    'meta.h',
    'peer.h',
    'uthash.h'
    ]
SOURCES = [
    'duplex.c',
    'peer.c'
    ]

def init(ctx):
    ctx.load('build_logs', tooldir='./waftools/')

def options(opt):
    opt.load('compiler_c')
    opt.load('gnu_dirs')
    opt.load('waf_unit_test')
    opt.add_option('--debug', default=False, dest='debug', action='store_true',
            help='Enable debug options -g -Wall -D_DEBUG')
    opt.add_option('--utils', default=False, dest='utils', action='store_true',
            help='Enable building utils')

def configure(conf):
    conf.load('compiler_c')
    conf.load('gnu_dirs')
    conf.load('waf_unit_test')

    conf.check(features='c cprogram', lib=['pthread'], uselib_store='PTHREAD')
    conf.check(features='c cprogram', lib=['uuid'], uselib_store='UUID')
    conf.check_cfg(package='libssh', uselib_store='LIBSSH',
                   atleast_version='0.6.0',
                   args='--cflags --libs')
    conf.check_cfg(package='libssh_threads', uselib_store='LIBSSH_THREADS',
                   atleast_version='0.6.0',
                   args='--cflags --libs')

    # for testing
    conf.check_cfg(package='check', uselib_store='CHECK',
                   args='--cflags --libs', mandatory=False)

    if conf.env.HAVE_CHECK != 1:
        print("warning: check is not found, unit tests are disabled.")

    if conf.options.debug:
        conf.env.append_unique('CFLAGS', ['-g', '-Wall', '-D_DEBUG'])

def build(bld):
    USE=['LIBSSH', 'LIBSSH_THREADS', 'PTHREAD', 'UUID']
    bld.stlib(source=SOURCES, target='duplex', use=USE)
    bld.shlib(source=SOURCES, target='duplex', vnum=VERSION, use=USE)
    bld(source='duplex.pc.in', version=VERSION)
    bld.install_files('${INCLUDEDIR}/duplex', HEADERS)
    bld.recurse('tests')

    if bld.options.utils:
        bld.recurse('utils')
