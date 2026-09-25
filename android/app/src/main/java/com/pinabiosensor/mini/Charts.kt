package com.pinabiosensor.mini

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Card
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.unit.dp

@Composable
fun SparklineCard(title: String, values: List<Float>, modifier: Modifier = Modifier) {
    val line = MaterialTheme.colorScheme.primary
    Card(modifier) {
        Text(title, style = MaterialTheme.typography.labelMedium, modifier = Modifier.padding(12.dp, 12.dp, 12.dp, 0.dp))
        if (values.size < 2) {
            Text("Esperando muestras…", style = MaterialTheme.typography.bodySmall, modifier = Modifier.padding(12.dp))
        } else {
            Canvas(
                Modifier
                    .fillMaxWidth()
                    .height(88.dp)
                    .padding(12.dp),
            ) {
                val min = values.min()
                val max = values.max()
                val span = (max - min).let { if (it < 1e-4f) 1f else it }
                val path = Path()
                values.forEachIndexed { i, v ->
                    val x = size.width * i / (values.size - 1).coerceAtLeast(1)
                    val y = size.height - (v - min) / span * size.height
                    if (i == 0) path.moveTo(x, y) else path.lineTo(x, y)
                }
                drawPath(path, line, style = Stroke(width = 3f, cap = StrokeCap.Round))
                val last = values.last()
                val lx = size.width
                val ly = size.height - (last - min) / span * size.height
                drawCircle(line, radius = 5f, center = Offset(lx, ly))
            }
        }
    }
}
