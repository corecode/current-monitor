# coding: utf-8
class BDF
  class Char
    attr_accessor :name, :encoding, :width, :bitmap

    def self.parse(lines)
      c = Char.new
      while !lines.empty?
        line = lines.shift
        fields = line.split
        case fields[0]
        when "STARTCHAR"
          c.name = fields[1]
        when "ENCODING"
          c.encoding = fields[1].to_i
        when "DWIDTH"
          c.width = fields[1].to_i
        when "BITMAP"
          c.bitmap = []
        when "ENDCHAR"
          return c, lines
        else
          if c.bitmap
            c.bitmap << [fields[0]].pack('H*').unpack('C*')
          end
        end
      end
    end

    def subset(*args)
      c = self.dup
      c.bitmap = c.bitmap.slice(*args)
      c
    end

    def show
      @bitmap.map do |l|
        pos = 0
        l.map do |e|
          8.times.map do |n|
            pos += 1
            e & (1 << (7-n)) != 0 ? 'x' : '.' if pos <= @width
          end.join
        end.join
      end.join("\n")
    end

    def vertical
      @width.times.map do |p|
        idx = p / 8
        bit = 7 - p % 8
        val = 0
        @bitmap.each_with_index do |horiz, vidx|
          set = horiz[idx] & (1 << bit) != 0 ? 1 : 0
          val |= set << vidx
        end
        val
      end
    end
  end

  attr_accessor :chars

  def initialize
    @chars = {}
  end

  def self.parse(lines)
    b = BDF.new
    while lines and !lines.empty?
      line = lines.shift
      fields = line.split
      case fields[0]
      when "CHARS"
        while lines
          c, lines = Char.parse(lines)
          b.chars[c.encoding] = c if c
        end
      end
    end
    b
  end
end

if $0 == __FILE__
  font = ARGV[0]
  yoff = Integer(ARGV[1])
  xoff = Integer(ARGV[2])

  name = File.basename(font, '.*').gsub(/[^[:alnum:]$]+/, '_')

  b = BDF.parse(File.readlines(font))

  exports = %w{0 1 2 3 4 5 6 7 8 9 m A \ }.map do |ch|
    glyph = b.chars[ch.ord]
    glyph.subset(yoff, 16)
  end

  exports << b.chars['µ'.ord].subset(7, 16)

  puts '#include "font.h"'
  puts ''
  puts "const struct font1610 #{name} = { #{exports.count}, {"
  exports.each do |g|
    data = g.vertical
    data = data[xoff,10]
    puts "  /* #{g.name} */"
    puts "  { #{g.encoding}, { #{data.join(", ")} }},"
  end
  puts "}};"
end
