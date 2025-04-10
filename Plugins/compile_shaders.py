#!/usr/bin/env python3
# Copyright MediaZ Teknoloji A.S. All Rights Reserved.


from subprocess import PIPE
import platform
from sys import stdout, stderr
import os
import sys
import threading
from loguru import logger
import subprocess

def run(*popenargs,
        input=None, capture_output=False, timeout=None, check=False, **kwargs) -> subprocess.CompletedProcess:
    # get the command line
    try:
        return subprocess.run(*popenargs, input=input, capture_output=capture_output, timeout=timeout, check=check, **kwargs)
    except Exception as e:
        cmd_list = popenargs[0]
        cmd = str(" ".join(cmd_list))
        logger.error(f"Command failed. \nCommand: \n\n {cmd}\n\nError: \n\n {e} \n\n")
        return subprocess.CompletedProcess(args=cmd, returncode=1, stdout="", stderr=str(e))
    
path = str(os.path.dirname(__file__))
global_ret_code = 0

def embed_binary(filepath):
    result = run([f"{path}/../Tools/{platform.system()}/bin2header.exe", filepath], stdout=PIPE, stderr=PIPE, universal_newlines=True)
    if result.returncode != 0:
        logger.warning(f"Failed to embed {filepath}")
        return result.returncode
    else:
        os.replace(filepath + ".h", filepath + ".dat")
        return 0

def compile_to_spv(filepath):
    logger.info(f"Compiling {filepath}")
    re = run([f"{path}/../Tools/{platform.system()}/glslc", "-o",  f"{filepath}_.spv", filepath], stdout=stdout, stderr=stderr, universal_newlines=True)
    if re.returncode != 0:
        logger.error(f"Failed to compile {filepath}")
        return re.returncode
    else:
        re = run([f"{path}/../Tools/{platform.system()}/spirv-opt", "-O", "-o",  f"{filepath}.spv", f"{filepath}_.spv"], stdout=stdout, stderr=stderr, universal_newlines=True)
        os.remove(f"{filepath}_.spv")
        if re.returncode != 0:
            logger.error(f"Failed to optimize {filepath}")
            return re.returncode
        else:
            return embed_binary(f"{filepath}.spv")
            # os.remove(f"{filepath}.spv")

def compile_shaders():
    def thread_func(filepath):
        global global_ret_code
        ret_code = compile_to_spv(filepath)
        if ret_code != 0:
            global_ret_code = ret_code

    compile_threads = []
    for root, dirs, files in os.walk(path):
        for filepath in files:
            if filepath.endswith(".frag") or filepath.endswith(".vert") or filepath.endswith(".comp"):
                fullpath = os.path.join(root, filepath)
                compiled = fullpath + ".spv.dat"
                if os.path.exists(compiled) and os.stat(fullpath).st_mtime <= os.stat(compiled).st_mtime:
                    logger.info(f"{filepath} is up to date.")
                    continue
                th = threading.Thread(target=thread_func,args=(fullpath,))
                th.start()
                compile_threads.append(th)
    for th in compile_threads:
        th.join()
    return global_ret_code

if __name__ == "__main__":
    logger.remove()
    logger.add(sys.stdout, format="<green>[Plugin Bundle Source]</green> <level>{time:HH:mm:ss.SSS}</level> <level>{level}</level> <level>{message}</level>")
    sys.exit(compile_shaders())
