package.cpath = package.cpath .. ";lib/?.so;lib/?.dylib;;"

local io = require "io"
local lio = require "lio"
local lpty = require "lpty"
local timeout = require "ltimeout"

local find = string.find
local concat = table.concat


local _M = {}

local mt = { __index = _M }


function _M.new(cols, rows, timeout, blocking)
    cols = tonumber(cols) or 128
    rows = tonumber(rows) or 64
    -- -1: no time limit
    timeout = tonumber(timeout) or -1

    local pty, err = lpty.open(cols, rows)
    if not pty then
        return nil, err
    end

    if blocking == false then
        local ok, err = lio.setnonblocking(pty.master)
        if not ok then
            return nil, err
        end

        ok, err = lio.setnonblocking(pty.slave)
        if not ok then
            return nil, err
        end
    end

    return setmetatable({
        cols = cols, rows = rows, timeout = timeout,
        master = pty.master, slave = pty.slave, name = pty.name,
        fresh = false, buffer = "",
    }, mt)
end


function _M.spawn(self, file, args, cwd)
    if not self.master then
        return nil, "no master"
    end

    self.fresh = true

    return lpty.spawn(self.master, self.slave, file, args,
                      { "PATH=/bin:/usr/bin:/usr/sbin:/usr/local/bin" },
                      cwd, self.cols, self.rows)
end


function _M.expect(self, pattern, timeout)
    if not self.fresh then
        return find(self.buf, pattern)
    end

    local buf = {}
    local try = (timeout or 1) / 0.1

    while try > 0 do
        local data, err = self:read(4096, 0.1)
        if data then
            io.write(data)
            buf[#buf + 1] = data
        elseif err == "timeout" then
            try = try - 1
        else
            return nil, err
        end
    end

    self.fresh = false
    self.buf = concat(buf)

    return find(self.buf, pattern)
end


function _M.play(self, pattern, data)
    if self:expect(pattern) then
        self:send(data)
    end
end


function _M.read(self, size, timeout)
    return lio.read(self.master, size, timeout or self.timeout)
end


function _M.write(self, data, timeout)
    self.fresh = true
    return lio.write(self.master, data, timeout or self.timeout)
end


_M.send = _M.write


function _M.interact(self)
end


function _M.clean(self)
    lio.destroy(self.master)
end


function _M.getfd(self)
    return self.master
end


function _M.dirty(self)
    return false
end


function _M.wait(self, time)
    lio.sleep(time)
end


return _M
