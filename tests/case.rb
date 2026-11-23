require 'minitest/autorun'
require 'open3'
require 'pty'
require 'timeout'

class ZeptoTest < Minitest::Test
  BINARY = File.expand_path('../zeptofetch', __dir__)
  ENVIRONMENT = { 'LC_ALL' => 'C.UTF-8', 'USER' => 'testuser', 'TERM' => 'testterm' }.freeze

  def run_fetch(*args, env: {})
    Timeout.timeout(10) do
      Open3.capture3(ENVIRONMENT.merge(env), BINARY, *args, unsetenv_others: true)
    end
  end

  def run_pty(*args, env: {})
    reader, writer, pid = PTY.spawn(ENVIRONMENT.merge(env), BINARY, *args, unsetenv_others: true)
    output = +''.b
    status = nil
    begin
      Timeout.timeout(10) do
        begin
          loop { output << reader.readpartial(4096) }
        rescue EOFError, Errno::EIO
        end
        _, status = Process.wait2(pid)
        pid = nil
      end
    ensure
      reader.close unless reader.closed?
      writer.close unless writer.closed?
      if pid
        begin
          Process.kill('KILL', pid)
        rescue Errno::ESRCH
        end
        Process.wait(pid) rescue Errno::ECHILD
      end
    end
    [output, status]
  end
end
