
require 'sinatra'

get '/' do

  'hello world'
end


# signal parent process that we are [almost] ready

s = `ps | grep "^ *#{Process.ppid}"`.strip

unless s.match(/ bash$/)

  puts "#{Process.pid} signalling #{Process.ppid}..."
  Process.kill('USR1', Process.ppid)
end

