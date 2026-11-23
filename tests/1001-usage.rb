require_relative 'case'

class UsageTest < ZeptoTest
  def test_invalid_arguments
    [['--invalid'], ['-v', 'extra'], ['extra', 'extra']].each do |args|
      out, err, status = run_fetch(*args)
      assert_equal 1, status.exitstatus
      assert_equal '', out
      assert_equal "Usage: zeptofetch [-v|--version]\n", err
    end
  end
end
