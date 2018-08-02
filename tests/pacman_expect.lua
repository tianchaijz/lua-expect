local Expect = require "expect"

local io = require "io"
local lio = require "lio"
local lpty = require "lpty"


local expect, err = Expect.new()
if not expect then
    error("expect new error: " .. err)
end

local ok, err = expect:spawn("sudo", { "pacman", "-S", "cloc" }, "/tmp")
if not ok then
    error("expect spawn error: " .. err)
end

expect:wait(1)

local stdin = {
    getfd = function()
        return 1
    end,

    dirty = function()
        return false
    end
}

local rset = { stdin, expect }
local wset = { expect }

local buf = ""
lpty.turn_echoing_off()
while true do
    local r, w, err = lio.select(rset, wset, 0)
    if err and err ~= "timeout" then
        error(err)
    end

    if #r > 0 then
        for _, obj in ipairs(r) do
            local data, err = lio.read(obj:getfd(), 4096, 0)
            if err == "closed" then
                return
            end

            if obj == stdin then
                buf = buf .. data
            elseif obj == expect then
                io.write(data)
                io.flush()
            end
        end
    end

    if #w > 0 and #buf > 0 then
        lio.write(expect:getfd(), buf, 0)
        buf = ""
    end

    lio.sleep(0.01)
end
