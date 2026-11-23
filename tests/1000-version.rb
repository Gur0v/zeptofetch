require_relative 'case'

class VersionTest < ZeptoTest
  def test_version_aliases
    long, long_err, long_status = run_fetch('--version')
    short, short_err, short_status = run_fetch('-v')
    assert long_status.success?
    assert short_status.success?
    assert_equal '', long_err
    assert_equal '', short_err
    assert_equal long, short
    assert_match(/\Azeptofetch v2\.3\nCopyright \(C\) \d{4}-\d{4} Gurov\nLicensed under GPL-3\.0\n\nBUILD: .+ UTC\nCONFIG: CHAIN=\d+ PID=\d+\n\z/, long)
  end
end
