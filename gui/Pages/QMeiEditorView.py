# -*- coding: utf-8 -*-
#
#-------------------------------------------------------------------------------
#
#     This file is part of the Code_Saturne User Interface, element of the
#     Code_Saturne CFD tool.
#
#     Copyright (C) 1998-2010 EDF S.A., France
#
#     contact: saturne-support@edf.fr
#
#     The Code_Saturne User Interface is free software; you can redistribute it
#     and/or modify it under the terms of the GNU General Public License
#     as published by the Free Software Foundation; either version 2 of
#     the License, or (at your option) any later version.
#
#     The Code_Saturne User Interface is distributed in the hope that it will be
#     useful, but WITHOUT ANY WARRANTY; without even the implied warranty
#     of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.
#
#     You should have received a copy of the GNU General Public License
#     along with the Code_Saturne Kernel; if not, write to the
#     Free Software Foundation, Inc.,
#     51 Franklin St, Fifth Floor,
#     Boston, MA  02110-1301  USA
#
#-------------------------------------------------------------------------------

"""
This module defines the Dialog window of the mathematical expression interpretor.

This module contains the following classes and function:
- format
- QMeiHighlighter
- QMeiEditorView
"""

#-------------------------------------------------------------------------------
# Library modules import
#-------------------------------------------------------------------------------

import sys, string
import logging

#-------------------------------------------------------------------------------
# Third-party modules
#-------------------------------------------------------------------------------

from PyQt4.QtCore import *
from PyQt4.QtGui  import *

#-------------------------------------------------------------------------------
# Application modules import
#-------------------------------------------------------------------------------

from Base.Toolbox import GuiParam
from QMeiEditorForm import Ui_QMeiDialog
import mei

#-------------------------------------------------------------------------------
# log config
#-------------------------------------------------------------------------------

logging.basicConfig()
log = logging.getLogger("QMeiEditorView")
#log.setLevel(GuiParam.DEBUG)
log.setLevel(logging.DEBUG)

#-------------------------------------------------------------------------------
# Syntax highlighter for the mathematical expressions editor.
#-------------------------------------------------------------------------------

def format(color, style=''):
    """Return a QTextCharFormat with the given attributes."""
    _color = QColor()
    _color.setNamedColor(color)

    _format = QTextCharFormat()
    _format.setForeground(_color)
    if 'bold' in style:
        _format.setFontWeight(QFont.Bold)
    if 'italic' in style:
        _format.setFontItalic(True)

    return _format


   # Syntax styles that can be shared by all expressions
STYLES = {
    'keyword': format('blue', 'bold'),
    'operator': format('red'),
    'brace': format('darkGray'),
    'required': format('magenta', 'bold'),
    'symbols': format('darkMagenta', 'bold'),
    'comment': format('darkGreen', 'italic'),
    }


class QMeiHighlighter(QSyntaxHighlighter):
    """
    Syntax highlighter for the mathematical expressions editor.
    """
    keywords = [
         'cos', 'sin', 'tan', 'exp', 'sqrt', 'log',
         'acos', 'asin', 'atan', 'atan2', 'cosh', 'sinh',
         'tanh', 'abs', 'mod', 'int', 'min', 'max',
         'pi', 'e', 'while', 'if', 'else', 'print',
         'raise', 'return', 'try', 'while', 'yield',
         'None', 'True', 'False',
    ]

    operators = [
        # logical
        '!', '==', '!=', '<', '<=', '>', '>=', '&&', '\|\|',
        # Arithmetic
        '=', '\+', '-', '\*', '/', '\^',
    ]

    braces = ['\{', '\}', '\(', '\)', '\[', '\]',]


    def __init__(self, document, required, symbols):
        QSyntaxHighlighter.__init__(self, document)

        rules = []
        rules += [(r'\b%s\b' % w, STYLES['keyword']) for w in QMeiHighlighter.keywords]
        rules += [(r'%s' % o, STYLES['operator']) for o in QMeiHighlighter.operators]
        rules += [(r'%s' % b, STYLES['brace']) for b in QMeiHighlighter.braces]
        rules += [(r'\b%s\b' % r, STYLES['required']) for r,l in required]
        rules += [(r'\b%s\b' % s, STYLES['symbols']) for s,l in symbols]
        rules += [(r'#[^\n]*', STYLES['comment'])]

        # Build a QRegExp for each pattern
        self.rules = [(QRegExp(pat), fmt) for (pat, fmt) in rules]


    def highlightBlock(self, text):
        """
        Apply syntax highlighting to the given block of text.
        """
        for rx, fmt in self.rules:
            pos = rx.indexIn(text, 0)
            while pos != -1:
                pos = rx.pos(0)
                s = rx.cap(0)
                self.setFormat(pos, s.length(), fmt)
                pos = rx.indexIn( text, pos+rx.matchedLength() )

#-------------------------------------------------------------------------------
# Dialog for mathematical expression interpretor
#-------------------------------------------------------------------------------

class QMeiEditorView(QDialog, Ui_QMeiDialog):
    """
    """
    def __init__(self, parent, expression = "", symbols = [], required = [], examples = ""):
        """
        Constructor.
        """
        QDialog.__init__(self, parent)

        Ui_QMeiDialog.__init__(self)
        self.setupUi(self)

        self.required = required
        self.symbols  = symbols

        # Syntax highlighting
        self.h1 = QMeiHighlighter(self.textEditExpression, required, symbols)
        self.h2 = QMeiHighlighter(self.textEditExamples, required, symbols)

        # Required symbols of the mathematical expression

        if not expression:
            expression = ""
            for p, q in required:
                expression += p + " = \n"
        else:
            while expression[0] in (" ", "\n", "\t"):
                expression = expression[1:]
            while expression[-1] in (" ", "\n", "\t"):
                expression = expression[:-1]

        # Predefined symbols for the mathematical expression

        predif = self.tr("<big><u>Required symbol:</u></big><br>")

        for p,q in required:
            for s in ("<b>", p, "</b>: ", q, "<br>"):
                predif += s

        if symbols:
            predif += self.tr("<br><big><u>Predefined symbols:</u></big><br>")

            for p,q in symbols:
                for s in ("<b>", p, "</b>: ", q, "<br>"):
                    predif += s

        predif += self.tr("<br>"\
                  "<big><u>Usefull functions:</u></big><br>"\
                  "<b>cos</b>: cosine<br>"\
                  "<b>sin</b>: sinus<br>"\
                  "<b>tan</b>: tangent<br>"\
                  "<b>exp</b>: exponential<br>"\
                  "<b>sqrt</b>: square root<br>"\
                  "<b>log</b>: napierian logarithm<br>"\
                  "<b>acos</b>: arccosine<br>"\
                  "<b>asin</b>: arcsine<br>"\
                  "<b>atan</b>: arctangent<br>"\
                  "<b>atan2</b>: arctangent<br>"\
                  "<b>cosh</b>: hyperbolic cosine<br>"\
                  "<b>sinh</b>: hyperbolic sine<br>"\
                  "<b>tanh</b>: hyperbolic tangent<br>"\
                  "<b>abs</b>: absolute value<br>"\
                  "<b>mod</b>: modulo<br>"\
                  "<b>int</b>: floor<br>"\
                  "<b>min</b>: minimum<br>"\
                  "<b>max</b>: maximum<br>"\
                  "<br>"\
                  "<big><u>Usefull constants:</u></big><br>"\
                  "<b>pi</b> = 3.14159265358979323846<br>"\
                  "<b>e</b> = 2.718281828459045235<br>"\
                  "<br>"\
                  "<big><u>Operators and statements:</u></big><br>"\
                  "<b><code> + - * / ^ </code></b><br>"\
                  "<b><code>! &lt; &gt; &lt;= &gt;= == != && || </code></b><br>"\
                  "<b><code>while if else</code></b><br>"\
                  "")

        # lay out the text

        self.textEditExpression.setText(expression)
        self.textEditSymbols.setText(predif)
        self.textEditExamples.setText(examples)

        self.expressionDoc = self.textEditExpression.document()


    @pyqtSignature("")
    def slotClearBackground(self):
        """
        Private slot.
        """
        QObject.disconnect(self.textEditExpression, SIGNAL("textChanged()"), self.slotClearBackground)
        doc = self.textEditExpression.document()

        for i in range(0, doc.blockCount()):
            block_text = doc.findBlockByNumber(i)
            log.debug("block: %s" % block_text.text())
            cursor = QTextCursor(block_text)
            block_format = QTextBlockFormat()
            block_format.clearBackground()
            cursor.setBlockFormat(block_format)


    def accept(self):
        """
        What to do when user clicks on 'OK'.
        """
        log.debug("accept()")

        doc = self.textEditExpression.document()

        intr = mei.mei_tree_new(str(self.textEditExpression.toPlainText()))
        log.debug("intr.string: %s" % intr.string)
        log.debug("intr.errors: %s" % intr.errors)

        for p,q in self.symbols:
            mei.mei_tree_insert(intr, p, 0.0)

        # Unknown symbols

        if mei.mei_tree_builder(intr):
            log.debug("intr.errors: %s" % intr.errors)
            for i in range(0, intr.errors):
                log.debug("  line:   %s \n  column: %s\n" % (intr.lines[i], intr.columns[i]))
            msg = ""
            for i in range(0, intr.errors):
                l = intr.lines[i]
                c = intr.columns[i]
                block_text = doc.findBlockByNumber(l - 1)
                cursor = QTextCursor(block_text)
                block_format = QTextBlockFormat()
                block_format.setBackground(QBrush(QColor(Qt.red)))
                cursor.setBlockFormat(block_format)

                #self.textEditExpression.setCursorPosition(l, c)
                #self.textEditExpression.setFocus()
                msg += intr.labels[i] + \
                       "    line: "   + str(l)   + " \n" + \
                       "    column: " + str(c) + " \n\n"

            QMessageBox.critical(self, self.tr('Expression Editor'), QString(msg))
            intr.errors = 0
            intr = mei.mei_tree_destroy(intr)
            QObject.connect(self.textEditExpression, SIGNAL("textChanged()"), self.slotClearBackground)
            return

        # If required symbols are missing

        list = []
        for p,q in self.required:
            list.append(p)

        if mei.mei_tree_find_symbols(intr, len(list), list):
            msg = "Warning, these required symbols are not found:\n\n"
            for i in range(0, doc.blockCount()):
                block_text = doc.findBlockByNumber(i)
                cursor = QTextCursor(block_text)
                block_format = QTextBlockFormat()
                block_format.setBackground(QBrush(QColor(Qt.yellow)))
                cursor.setBlockFormat(block_format)
                #self.textEditExpression.setFocus()
            for i in range(0, intr.errors): msg += intr.labels[i] + " \n"
            QMessageBox.critical(self, self.tr('Expression Editor'), QString(msg))
            intr.errors = 0
            intr = mei.mei_tree_destroy(intr)
            QObject.connect(self.textEditExpression, SIGNAL("textChanged()"), self.slotClearBackground)
            return

        intr = mei.mei_tree_destroy(intr)

        QDialog.accept(self)


    def reject(self):
        """
        Method called when 'Cancel' button is clicked
        """
        log.debug("reject()")
        QDialog.reject(self)


    def get_result(self):
        """
        Method to get the result
        """
        return self.textEditExpression.toPlainText()


    def tr(self, text):
        """
        Translation
        """
        return text

#-------------------------------------------------------------------------------
# Test function
#-------------------------------------------------------------------------------

if __name__ == "__main__":
    import sys, signal
    app = QApplication(sys.argv)
    app.connect(app, SIGNAL("lastWindowClosed()"), app, SLOT("quit()"))
    parent = QWidget()
    dlg = QMeiEditorView(parent)
    dlg.show()
    signal.signal(signal.SIGINT, signal.SIG_DFL)
    sys.exit(app.exec_())

#-------------------------------------------------------------------------------
# End
#-------------------------------------------------------------------------------
