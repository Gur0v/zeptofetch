require_relative 'case'

class TtyTest < ZeptoTest
  def test_terminal_colors
    output, status = run_pty
    assert status.success?
    assert_includes output, "\e[1;34m".b
    assert_includes output, "\e[0m".b
    plain = output.gsub(/\e\[[0-9;]*m/n, '').gsub("\r\n".b, "\n".b)
    assert_includes plain, "testuser@".b
    %w[OS Kernel Shell WM Terminal].each do |label|
      assert_includes plain, "#{label}: ".b
    end
    assert plain.end_with?("\n\n".b)
  end
end
