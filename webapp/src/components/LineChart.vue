<script>
import { Line, mixins } from 'vue-chartjs'
const { reactiveProp } = mixins
const options = {
  events: ['click'],
}

export default {
  extends: Line,
  mixins: [reactiveProp],
  props: ['chartData', 'options'],
  mounted () {
    this.renderChart(this.chartData, this.options)
  }
}
</script>
