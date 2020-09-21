import pathlib
import subprocess
import datetime


def make_local_git_command(repo_path):
    def __git_command(arg_list):
        return subprocess.check_output(["git", "-C", repo_path] + arg_list).strip()

    return __git_command


base_dir = pathlib.Path(__file__).parent.absolute() / "../source"


git = make_local_git_command(base_dir)


git(["checkout", "master"])
git(["pull", "origin", "master"])
git(["fetch", "--tags"])
current_version = git(["describe", "--tags", "--abbrev=0"]).decode("utf-8").split(".")
(major, minor, subminor, revision) = current_version[0:4]


today = datetime.date.today()


version_path = base_dir / "version.hpp"

if int(major) != today.year or int(minor) != today.month or int(subminor) != today.day:
    revision = 0
else:
    revision = int(revision) + 1


with open(version_path, "w") as version_file:
    version_file.write("#pragma once\n")

    version_file.write("#define PROGRAM_MAJOR_VERSION {}\n".format(today.year))
    version_file.write("#define PROGRAM_MINOR_VERSION {}\n".format(today.month))
    version_file.write("#define PROGRAM_SUBMINOR_VERSION {}\n".format(today.day))
    version_file.write("#define PROGRAM_VERSION_REVISION {}\n".format(revision))


git(["add", version_path])
git(["commit", "-m", '"bump version"'])

str_version = "{}.{}.{}.{}".format(today.year, today.month, today.day, revision)
git(["tag", "-a", str_version, '-m', 'release version {}'.format(str_version)])

git(["push", "--follow-tags", "origin", "master"])
