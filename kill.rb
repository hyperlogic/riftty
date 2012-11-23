#!/usr/bin/ruby

# kill all running riftty processes

`ps -la`.each_line do |line|
  tokens = line.split(/\s+/)
  pid = tokens[2]
  cmd = tokens[15]
  if (tokens[15] == './riftty')
    `kill -9 #{pid}`
  end
end
