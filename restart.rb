#!/usr/bin/ruby
require 'date'

# return 1 if there is only one riftty process.
# return 0 if there is already a riftty process, and restart it.

def get_procs
  procs = []
  `ps -caopid,comm,lstart`.each_line do |line|
    match = /\s*(\S*)\s*(\S*)\s*(.*)/.match(line)
    pid, comm, lstart = match[1], match[2], match[3]
    if pid != 'PID'
      time = DateTime.strptime(lstart, "%c")
      procs.push({ time: time, pid: pid, comm: comm })
    end
  end
  procs
end

procs = get_procs.select {|m| m[:comm] == 'riftty'}.sort_by {|m| m[:time]}

# debug
procs.each {|m| puts m.inspect}

if procs.size == 2
  # send sigusr1 to old process
  `kill -30 #{procs[0][:pid]}`
  exit 0  # success
else
  exit 1  # failure
end

