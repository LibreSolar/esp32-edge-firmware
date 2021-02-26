<template>
  <v-container  v-if="$vuetify.breakpoint.lgAndUp" fill-height>
    <v-layout text-center>
      <v-flex>
        <v-card >
          <v-card-title primary-title class="justify-center">Live View</v-card-title>
          <v-card-text>
            <v-row justify="center" dense>
                <v-col cols=8>
                    <v-select
                    v-model="selected_data"
                    :items="$store.state.chart_data_keys"
                    :menu-props="{ maxHeight: '400' }"
                    label="Select"
                    multiple
                    chips
                    hint="Pick data nodes for display"
                    persistent-hint
                    ></v-select>
                </v-col>
            </v-row>
            <v-row justify="center" dense>
                <v-spacer></v-spacer>
                <v-col cols=8>
                    <v-btn @click="clear_selection()">Clear Selection</v-btn></v-col>
                <v-spacer></v-spacer>
            </v-row>

            <v-row justify="center" class="flex-grow-0">
                <v-col cols=12 >
                  <line-chart :chart-data="storedata" :height="180" :responsive=false :options="options"></line-chart>
                </v-col>
            </v-row>
          </v-card-text>
        </v-card>
      </v-flex>
    </v-layout>
  </v-container>
  <v-container  v-else fluid fill-height pa-0 ma-0>
    <v-layout text-center align-center>
      <v-flex>
        <div>
          <v-select
            v-model="selected_data"
            :items="$store.state.chart_data_keys"
            :menu-props="{ maxHeight: '400' }"
            label="Select"
            multiple
            outlined
            dense
            hint="Pick data nodes for display"
            persistent-hint
            ></v-select>
            <v-btn @click="clear_selection()">Clear Selection</v-btn>
          <line-chart :chart-data="storedata" :responsive=true :options="options"></line-chart>
        </div>
      </v-flex>
    </v-layout>
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
	purple: 'rgb(153, 102, 255)'
};


export default {
  components: {
    LineChart
  },
  data () {
    return {
      selected_data: [],
      plot_height: 0,
      storedata: null,
      labels: [],
      run: true,
      timer: null,
      intervall: 3,
      num_data_points: 100,
      options: {
        scales: {
            xAxes: [{
              display: true,
              scaleLabel: {
                display: true,
                labelString: 'Minutes elapsed',
                fontSize: 18
              },
              ticks: {
                fontSize: 16
            }
            }],
            yAxes: [{
              ticks: {
                fontSize: 16
            }
            }]
        }
      }
    }
  },
  computed: {
  },
  created() {
    this.plot_height = 400;
  },
  mounted() {
    this.$store.dispatch('init_chart_data');
    clearInterval(this.timer);
    this.timer = setInterval(this.getData, this.intervall*1000);
    this.makeLabels()
  },
  beforeDestroy() {
    clearInterval(this.timer);
  },
  methods: {
    clear_selection() {
      this.selected_data = []
    },
    updateGraph() {
      let data = []
      this.selected_data.forEach((key, index) => {
        let buf = new Object()
        let colorNames = Object.keys(chartColors)
        buf["label"] = this.$store.state.info.output[key].title.en
        buf["fill"] = false
        buf["pointRadius"] = 2
        buf["lineTension"] = 0
        buf["borderColor"] = chartColors[colorNames[index % colorNames.length]]
        buf["data"] = this.$store.state.chart_data[key]
        data.push(buf)
      })
      this.storedata = {
        labels: this.labels,
        datasets: data
      }
    },
    makeLabels() {
      this.labels = Array(this.num_data_points)
      for(var i = -this.num_data_points; i != 1; i++) {
          this.labels[this.num_data_points + i] = "" + (i / 20)
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