
require 'pp'
require 'sinatra'


get '/' do

  "**hello world**\n"
end

get '/mirror' do

  "GET #{env['PATH_INFO']} HTTP/1.1\r\n" +
  env.select { |k, v|
    k[0] >= 'A' && k[0] <= 'Z'
  }.collect { |k, v|
    "#{k}: #{v}"
  }.join("\r\n") +
  "\r\n"
end

delete '/d' do

  "deleted."
end

