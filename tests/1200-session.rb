require_relative 'case'
require 'rbconfig'

class SessionTest < ZeptoTest
  def test_headless_container
    skip 'Linux container marker only' unless RbConfig::CONFIG['host_os'].include?('linux')
    out, err, status = run_fetch(env: { 'CONTAINER_ID' => 'test', 'SSH_CONNECTION' => 'test' })
    assert status.success?, err
    assert_match(/WM: none\n/, out)
    assert_match(/Terminal: container\n/, out)
  end

  def test_terminal_hint_precedence
    env = { 'DISPLAY' => ':99', 'KITTY_PID' => '42', 'TERM_PROGRAM' => 'other', 'SWAYSOCK' => 'test' }
    out, err, status = run_fetch(env: env)
    assert status.success?, err
    assert_match(/Terminal: kitty\n/, out)
    assert_match(/WM: sway\n/, out)
  end

  def test_empty_hint_does_not_override
    out, err, status = run_fetch(env: { 'DISPLAY' => ':99', 'KITTY_PID' => '', 'TERM_PROGRAM' => 'example' })
    assert status.success?, err
    assert_match(/Terminal: example\n/, out)
  end
end
