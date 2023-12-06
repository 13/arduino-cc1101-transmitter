#!/usr/bin/env python

import subprocess
import datetime

# Run the git describe command
git_version = subprocess.check_output(['git', 'describe', '--abbrev=7', '--always', '--tags']).decode('utf-8').strip()

# Get current date and time
build_time = datetime.datetime.now().strftime("%Y-%m-%dT%H:%M:%S")

# Print the results
version = git_version + ' (' + build_time + ')'
version_tag = git_version.split('-')[0]

print("'-DVERSION=\"%s\"'" % version)
#print("'-DVERSIONTAG=\"%s\"'" % version_tag)

#print("'-DVERSION=\"%s\"\n-DVERSIONTAG=\"%s\"'" % (version, version_tag))
