<template>
  <v-container>
    <v-row>
      <v-col cols=2><v-btn @click="run = !run">Stop</v-btn></v-col>
      <v-col cols=8>
        <line-chart :chart-data="storedata" :height="270" :responsive=false></line-chart>
      </v-col>
      <v-col cols=2></v-col>
    </v-row>
  </v-container>
</template>

<script>
import LineChart from '../components/LineChart'

const chartColors = {
	red: 'rgb(255, 99, 132)',
	orange: 'rgb(255, 159, 64)',
	yellow: 'rgb(255, 205, 86)',
	green: 'rgb(75, 192, 192)',
	blue: 'rgb(54, 162, 235)',
	purple: 'rgb(153, 102, 255)',
	grey: 'rgb(201, 203, 207)'
};

export default {
  components: {
    LineChart
  },
  data () {
    return {
      storedata: null,
      run: true,
      timer: null
    }
  },
  computed: {
  },
  mounted () {
    this.$store.dispatch('init_chart_data');
    clearInterval(this.timer);
    this.timer = setInterval(this.getData, 3000);
  },
  beforeUnmount() {
    clearInterval(this.timer);
  },
  methods: {
    updateGraph() {
      let data = []
      this.$store.state.chart_data_keys.forEach((key, index) => {
        let buf = new Object()
        let colorNames = Object.keys(chartColors)
        buf["label"] = key
        buf["fill"] = false
        buf["pointRadius"] = 2
        buf["lineTension"] = 0
        buf["borderColor"] = chartColors[colorNames[index]]
        buf["data"] = this.$store.state.chart_data[key]
        data.push(buf)
      })
      this.storedata = {
        labels: Array(20).fill("Data"),
        datasets: data
      }
    },
    async getData () {
      if (this.run) {
        await this.$store.dispatch('update_chart_data');
        this.updateGraph();
      }
    }
  }
}
</script>