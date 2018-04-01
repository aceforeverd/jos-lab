cmake_minimum_required(VERSION 3.10)
project(jos_2018_spring)

set(CMAKE_CXX_STANDARD 11)

include_directories(inc)
include_directories(kern)

add_executable(jos_2018_spring
        boot/boot.S
        boot/main.c
        conf/env.mk
        conf/lab.mk
        inc/assert.h
        inc/elf.h
        inc/error.h
        inc/kbdreg.h
        inc/memlayout.h
        inc/mmu.h
        inc/queue.h
        inc/stab.h
        inc/stdarg.h
        inc/stdio.h
        inc/string.h
        inc/types.h
        inc/x86.h
        kern/console.c
        kern/console.h
        kern/entry.S
        kern/entrypgdir.c
        kern/init.c
        kern/kclock.c
        kern/kclock.h
        kern/kdebug.c
        kern/kdebug.h
        kern/kernel.ld
        kern/Makefrag
        kern/monitor.c
        kern/monitor.h
        kern/pmap.c
        kern/pmap.h
        kern/printf.c
        lib/printfmt.c
        lib/readline.c
        lib/string.c)
