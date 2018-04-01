cmake_minimum_required(VERSION 3.9)
project(jos_2018_spring)

set(CMAKE_CXX_STANDARD 11)

add_executable(jos_2018_spring
        boot/boot.S
        boot/main.c
        inc/assert.h
        inc/elf.h
        inc/error.h
        inc/kbdreg.h
        inc/malloc.h
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
        kern/kdebug.c
        kern/kdebug.h
        kern/monitor.c
        kern/monitor.h
        kern/printf.c
        lib/printfmt.c
        lib/readline.c
        lib/string.c)
