require_relative 'case'

class OutputErrorsTest < ZeptoTest
  def test_full_device
    skip '/dev/full unavailable' unless File.chardev?('/dev/full')
    [[], ['--version']].each do |args|
      pid = Process.spawn(ENVIRONMENT, BINARY, *args, unsetenv_others: true, out: '/dev/full', err: File::NULL)
      _, status = Timeout.timeout(10) { Process.wait2(pid) }
      refute status.success?
    end
  end
end
