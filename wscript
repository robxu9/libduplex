#! /usr/bin/env python
# encoding: utf-8

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
SOURCES = 'duplex.c'

def options(opt):
    opt.load('compiler_c')
    opt.load('gnu_dirs')
    opt.load('cflags', tooldir='./waftools/')


def configure(conf):
    conf.load('compiler_c')
    conf.load('gnu_dirs')
    conf.load('cflags', tooldir='./waftools/')
    conf.check(features='c cprogram', lib=['pthread'], uselib_store='PTHREAD')
    conf.check(features='c cprogram', lib=['uuid'], uselib_store='UUID')
    conf.check_cfg(package='libssh', uselib_store='LIBSSH',
                   atleast_version='0.6.0',
                   args='--cflags --libs')
    conf.check_cfg(package='libssh_threads', uselib_store='LIBSSH_THREADS',
                   atleast_version='0.6.0',
                   args='--cflags --libs')

def build(bld):
    USE=['LIBSSH', 'LIBSSH_THREADS', 'PTHREAD', 'UUID']
    bld.stlib(source=SOURCES, target='duplex', use=USE)
    bld.shlib(source=SOURCES, target='duplex', vnum=VERSION, use=USE)
    bld(source='duplex.pc.in', version=VERSION)
    bld.install_files('${INCLUDEDIR}/libduplex', HEADERS)
