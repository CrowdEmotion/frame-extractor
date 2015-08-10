import subprocess as subp
import shutil
import time
import csv
from os import listdir, makedirs
from os.path import isfile, join, getmtime, exists

def copyFolderContents(src, dest):
    if not exists(dest):
        makedirs(dest)

    onlyfiles = [ f for f in listdir(src) if isfile(join(src,f)) ]

    for file_name in onlyfiles:
        file_path = join(src, file_name)
        #if not exists(file_path):
        shutil.copy(file_path, dest)


# main
if __name__ == '__main__':
    data = "data/"
    frames = join(data, "frames/")
    agregate = "agregates/"
    persistent = "dictionary.csv"
    dict = {}

    if exists(persistent):
        with open(persistent) as csvFile:
            for key, val in csv.reader(csvFile):
                dict[key] = val

    subp.call(["java", "-jar", join(data, "SyncSSH/SyncSSH.jar"), "download", join(data, "SyncSSH")])

    onlyfiles = [ f for f in listdir(data) if isfile(join(data,f)) ]

    for file_name in onlyfiles:
        if not file_name.endswith(".mp4"):
            continue;

        dfile_name = file_name[:-4] + ".txt"
        file_path = join(data, file_name)
        dfile_path = join(data, dfile_name)

        if not exists(dfile_path):
            print "file " + file_path + " does not have a dfile"
            continue;
        
        if file_name in dict and dict[file_name] == getmtime(file_path):
            if dfile_name in dict and dict[dfile_name] == getmtime(file_path):
                print "ignored file: " + file_path + " because it's not new"
                continue;
        if subp.call(["fe.exe", file_path, dfile_path]) == 0:
            dict[file_name] = str(getmtime(file_path))
            dict[dfile_name] = str(getmtime(dfile_path))

    open(persistent, 'w').close()
    with open(persistent, "w") as csvFile:
        w = csv.writer(csvFile)
        for key, val in dict.items():
            w.writerow([key, val])

    folders = [ f for f in listdir(frames) if not isfile(join(frames,f)) ]

    for folder_name in folders:
        folder_path = join(frames, folder_name)
        subfolders = [ f for f in listdir(folder_path) if not isfile(join(folder_path,f)) ]

        for subf_name in subfolders:
            src = join(folder_path, subf_name)
            dst = join(agregate, subf_name)

            copyFolderContents(src, dst)

    subp.call(["java", "-jar", join(data, "SyncSSH/SyncSSH.jar"), "upload", join(data, "SyncSSH")])

