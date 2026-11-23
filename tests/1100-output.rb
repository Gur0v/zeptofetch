require_relative 'case'

class OutputTest < ZeptoTest
  def test_plain_layout
    out, err, status = run_fetch
    assert status.success?, err
    refute_includes out, "\e"
    lines = out.lines
    assert_equal 8, lines.length
    assert_match(/\A    ___      testuser@.+\n\z/, lines[0])
    assert_match(/\A   \(\.· \|     -+\n\z/, lines[1])
    %w[OS Kernel Shell WM Terminal].each_with_index do |label, i|
      assert_match(/#{label}: .+\n\z/, lines[i + 2])
    end
    assert_equal "\n", lines[7]
  end

  def test_unicode_and_controls
    out, err, status = run_fetch(env: { 'USER' => "José\e[31m" })
    assert status.success?, err
    assert_includes out, "José_[31m@"
    refute_includes out, "\e"
  end
end
