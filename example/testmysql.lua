local driver  = require "luasql.mysql"

--创建环境对象
env = driver.mysql()

--连接数据库
conn = env:connect("ias_test","root","802802","127.0.0.1",3306)

--设置数据库的编码格式
conn:execute"SET NAMES GB2312"

--[[
function rows (connection, sql_statement)
  local cursor = assert (connection:execute (sql_statement))
  return function ()
    return cursor:fetch()
  end
end

for id,UserName in rows(conn ,"SELECT id,UserName from user") do
    print(string.format("%s  %s",id,UserName))
end
]]--

--执行数据库操作
cur = conn:execute("select id,UserName from user")

print(cur:numrows())

row = cur:fetch("a")

while row do
    print(row.id,row.UserName)
    row = cur:fetch("a")
end

conn:close()  --关闭数据库连接
env:close()   --关闭数据库环境
