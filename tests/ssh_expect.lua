local Expect = require "expect"


local expect, err = Expect.new()
if not expect then
    error("expect new error: " .. err)
end

local ok, err = expect:spawn("ssh", { "ssh@127.0.0.1", "-p", "22" }, "/tmp")
if not ok then
    error("expect spawn error: " .. err)
end

expect:wait(1)

expect:play("yes/no", "yes\r")
expect:play("password", "ssh\r")

ok, err = expect:expect("Last login:", 2)
if not ok then
    error(err or "login failed")
end

expect:send("ls -al $HOME && pwd\r")
expect:play("/home/ssh", "exit\r")
expect:expect("logout")
